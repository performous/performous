#include "audio.hh"
#include "configuration.hh"

#include "fs.hh"
#include "util.hh"
#include "i18n.hh"
#include "screen_intro.hh"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <libxml++/libxml++.h>
#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <cmath>

Config config;

ConfigItem::ConfigItem(bool bval): m_type("bool"), m_value(bval), m_sel() { }

ConfigItem::ConfigItem(int ival): m_type("int"), m_value(ival), m_sel() { }

ConfigItem::ConfigItem(float fval): m_type("float"), m_value(fval), m_sel() { }

ConfigItem::ConfigItem(std::string sval): m_type("string"), m_value(sval), m_sel() { }

ConfigItem::ConfigItem(OptionList opts): m_type("option_list"), m_value(opts), m_sel() { }


ConfigItem& ConfigItem::incdec(int dir) {
    if (m_type == "int") {
        int& val = boost::get<int>(m_value);
        int step = boost::get<int>(m_step);
        val = clamp(((val + dir * step)/ step) * step, boost::get<int>(m_min), boost::get<int>(m_max));
    } else if (m_type == "float") {
        double& val = boost::get<double>(m_value);
        double step = boost::get<double>(m_step);
        val = clamp(round((val + dir * step) / step) * step, boost::get<double>(m_min), boost::get<double>(m_max));
    } else if (m_type == "bool") {
        bool& val = boost::get<bool>(m_value);
        val = !val;
    } else if (m_type == "option_list") {
        size_t s = boost::get<OptionList>(m_value).size();
        m_sel = (m_sel + dir + s) % s;
    }
    return *this;
}

bool ConfigItem::isDefaultImpl(ConfigItem::Value const& defaultValue) const {
    if (m_type == "bool") return boost::get<bool>(m_value) == boost::get<bool>(defaultValue);
    if (m_type == "int") return boost::get<int>(m_value) == boost::get<int>(defaultValue);
    if (m_type == "float") return boost::get<double>(m_value) == boost::get<double>(defaultValue);
    if (m_type == "string") return boost::get<std::string>(m_value) == boost::get<std::string>(defaultValue);
    if (m_type == "string_list") return boost::get<StringList>(m_value) == boost::get<StringList>(defaultValue);
    if (m_type == "option_list") return boost::get<OptionList>(m_value) == boost::get<OptionList>(defaultValue);
    throw std::logic_error("ConfigItem::is_default doesn't know type '" + m_type + "'");
}

void ConfigItem::verifyType(std::string const& type) const {
    if (type == m_type) return;
    std::string name = "unknown";
    // Try to find this item in the config map
    for (Config::const_iterator it = config.begin(); it != config.end(); ++it) {
        if (&it->second == this) { name = it->first; break; }
    }
    if (m_type.empty()) throw std::logic_error("Config item " + name + ", requested_type=" + type + " used in C++ but missing from config schema");
    throw std::logic_error("Config item type mismatch: item=" + name + ", type=" + m_type + ", requested=" + type);
}

int& ConfigItem::i() { verifyType("int"); return boost::get<int>(m_value); }
int const& ConfigItem::i() const { verifyType("int"); return boost::get<int>(m_value); }
bool& ConfigItem::b() { verifyType("bool"); return boost::get<bool>(m_value); }
double& ConfigItem::f() { verifyType("float"); return boost::get<double>(m_value); }
std::string& ConfigItem::s() { verifyType("string"); return boost::get<std::string>(m_value); }
ConfigItem::StringList& ConfigItem::sl() { verifyType("string_list"); return boost::get<StringList>(m_value); }
ConfigItem::OptionList& ConfigItem::ol() { verifyType("option_list"); return boost::get<OptionList>(m_value); }
std::string& ConfigItem::so() { verifyType("option_list"); return boost::get<OptionList>(m_value).at(m_sel); }

void ConfigItem::select(int i) { verifyType("option_list"); m_sel = clamp<int>(i, 0, boost::get<OptionList>(m_value).size()-1); }

namespace {
    template <typename T, typename VariantAll, typename VariantNum> std::string numericFormat(VariantAll const& value, VariantNum const& multiplier, VariantNum const& step) {
        T m = boost::get<T>(multiplier);
        // Find suitable precision (not very useful for integers, but this code is generic...)
        T s = std::abs(m * boost::get<T>(step));
        unsigned precision = 0;
        while (s > 0.0 && (s *= 10) < 10) ++precision;
        // Format the output
        boost::format fmter("%f");
        fmter % boost::io::group(std::setprecision(precision), double(m) * boost::get<T>(value));
        return fmter.str();
    }
    
    std::string getText(xmlpp::Element const& elem) {
        xmlpp::TextNode const* n = elem.get_child_text();  // Returns NULL if there is no text
        return n ? std::string(n->get_content()) : std::string();
    }
    
    std::string getText(xmlpp::Element const& elem, std::string const& path) {
        xmlpp::NodeSet ns = elem.find(path);
        if (ns.empty()) return std::string();
        return getText(dynamic_cast<xmlpp::Element const&>(*ns[0]));
    }
}

std::string const ConfigItem::getValue() const {
    if (this->getShortDesc() == config["audio/backend"].getShortDesc()) {
        int AutoBackendType = 1337;
        static int val = boost::get<int>(m_value);
        if (val != boost::get<int>(m_value)) val = PaHostApiNameToHostApiTypeId(this->getEnumName()); // In the case of the audio backend, val is the real value while m_value is the enum case for its cosmetic name.
        std::clog << "audio/debug: Value of selected audio backend in config.xml is: " << val << std::endl;
        int hostApi = Pa_HostApiTypeIdToHostApiIndex(PaHostApiTypeId(val));
        std::ostringstream oss;
        oss << "audio/debug: Trying the selected Portaudio backend...";
        if (val != AutoBackendType) {
            oss << " found at index: " << hostApi;
        }
        else {
            oss << " not found; but this is normal when Auto is selected."; // Auto is not a real PaHostApiTypeId, so it will always return paHostApiNotFound
        }
        oss << std::endl;
        std::clog << oss.str();
        if ((hostApi != paHostApiNotFound) || (val == AutoBackendType)) {
            std::string backendName = (val != AutoBackendType) ? Pa_GetHostApiInfo(hostApi)->name : "Auto";
            std::clog << "audio/info: Currently selected audio backend is: " << backendName << std::endl;
            return backendName;
        }
        else std::clog << "audio/warning: Currently selected audio backend is unavailable on this system, will default to Auto." << std::endl;
        return "Auto";
    }
    if (m_type == "int") {
        int val = boost::get<int>(m_value);
        if (val >= 0 && val < int(m_enums.size())) return m_enums[val];
        return numericFormat<int>(m_value, m_multiplier, m_step) + m_unit;
    }
    if (m_type == "float") return numericFormat<double>(m_value, m_multiplier, m_step) + m_unit;
    if (m_type == "bool") return boost::get<bool>(m_value) ? _("Enabled") : _("Disabled");
    if (m_type == "string") return boost::get<std::string>(m_value);
    if (m_type == "string_list") {
        StringList const& sl = boost::get<StringList>(m_value);
        return sl.size() == 1 ? "{" + sl[0] + "}" : (boost::format(_("%d items")) % sl.size()).str();
    }
    if (m_type == "option_list") return boost::get<OptionList>(m_value).at(m_sel);
    throw std::logic_error("ConfigItem::getValue doesn't know type '" + m_type + "'");
}

namespace {
    struct XMLError {
        XMLError(xmlpp::Element& e, std::string msg): elem(e), message(msg) {}
        xmlpp::Element& elem;
        std::string message;
    };
    std::string getAttribute(xmlpp::Element& elem, std::string const& attr) {
        xmlpp::Attribute* a = elem.get_attribute(attr);
        if (!a) throw XMLError(elem, "attribute " + attr + " not found");
        return a->get_value();
    }
    template <typename T, typename V> void setLimits(xmlpp::Element& e, V& min, V& max, V& step) {
        xmlpp::Attribute* a = e.get_attribute("min");
        if (a) min = boost::lexical_cast<T>(a->get_value());
        a = e.get_attribute("max");
        if (a) max = boost::lexical_cast<T>(a->get_value());
        a = e.get_attribute("step");
        if (a) step = boost::lexical_cast<T>(a->get_value());
    }
}

void ConfigItem::addEnum(std::string name) {
    verifyType("int");
    if (find(m_enums.begin(),m_enums.end(),name) == m_enums.end()) {
        m_enums.push_back(name);
    }
    m_min = 0;
    m_max = int(m_enums.size() - 1);
    m_step = 1;
}

void ConfigItem::selectEnum(std::string const& name) {
    auto it = std::find(m_enums.begin(), m_enums.end(), name);
    if (it == m_enums.end()) throw std::runtime_error("Enum value " + name + " not found in " + m_shortDesc);
    i() = it - m_enums.begin();
}


std::string const ConfigItem::getEnumName() const {
    int const& val = i();
    if (val >= 0 && val < int(m_enums.size())) { return m_enums[val]; }
    else { return std::string(); }
}

template <typename T> void ConfigItem::updateNumeric(xmlpp::Element& elem, int mode) {
    xmlpp::NodeSet ns = elem.find("limits");
    if (!ns.empty()) setLimits<T>(dynamic_cast<xmlpp::Element&>(*ns[0]), m_min, m_max, m_step);
    else if (mode == 0) throw XMLError(elem, "child element limits missing");
    ns = elem.find("ui");
    // Default values
    if (mode == 0) {
        m_unit.clear();
        m_multiplier = static_cast<T>(1);
    }
    if (!ns.empty()) {
        xmlpp::Element& e = dynamic_cast<xmlpp::Element&>(*ns[0]);
        try { m_unit = getAttribute(e, "unit"); } catch (...) {}
        std::string m;
        try {
            m = getAttribute(e, "multiplier");
            m_multiplier = boost::lexical_cast<T>(m);
        } catch (XMLError&) {}
        catch (boost::bad_lexical_cast&) { throw XMLError(e, "attribute multiplier='" + m + "' value invalid"); }
    }
}


void ConfigItem::update(xmlpp::Element& elem, int mode) try {
    if (mode == 0) {
        m_type = getAttribute(elem, "type");
        if (m_type.empty()) throw std::runtime_error("Entry type attribute is missing");
        // Menu text
        m_shortDesc = getText(elem, "short");
        m_longDesc = getText(elem, "long");
    } else {
        std::string type = getAttribute(elem, "type");
        if (!type.empty() && type != m_type) throw std::runtime_error("Entry type mismatch: " + getAttribute(elem, "name") + ": schema type = " + m_type + ", config type = " + type);
    }
    if (m_type == "bool") {
        std::string value_string = getAttribute(elem, "value");
        bool value;
        if (value_string == "true") value = true;
            else if (value_string == "false") value = false;
                else throw std::runtime_error("Invalid boolean value '" + value_string + "'");
        m_value = value;
    } else if (m_type == "int") {
        std::string value_string = getAttribute(elem, "value");
        if (!value_string.empty()) m_value = boost::lexical_cast<int>(value_string);
            // Enum handling
            if (mode == 0) {
                xmlpp::NodeSet n2 = elem.find("limits/enum");
                if (!n2.empty()) {
                    for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
                        xmlpp::Element& elem2 = dynamic_cast<xmlpp::Element&>(**it2);
                        m_enums.push_back(getText(elem2));
                    }
                    m_min = 0;
                    m_max = int(m_enums.size() - 1);
                    m_step = 1;
                }
            }
        updateNumeric<int>(elem, mode);
    } else if (m_type == "float") {
        std::string value_string = getAttribute(elem, "value");
        if (!value_string.empty()) m_value = boost::lexical_cast<double>(value_string);
            updateNumeric<double>(elem, mode);
            } else if (m_type == "string") {
                m_value = getText(elem, "stringvalue");
            } else if (m_type == "string_list" || m_type == "option_list") {
                //TODO: Option list should also update selection (from attribute?)
                std::vector<std::string> value;
                xmlpp::NodeSet n2 = elem.find("stringvalue");
                for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
                    value.push_back(getText(dynamic_cast<xmlpp::Element const&>(**it2)));
                }
                m_value = value;
            } else if (!m_type.empty()) throw std::runtime_error("Invalid value type in config schema: " + m_type);
    // Schema sets all defaults, system config sets the system default
    if (mode < 1) m_factoryDefaultValue = m_defaultValue = m_value;
        if (mode < 2) m_defaultValue = m_value;
            } catch (std::exception& e) {
                int line = elem.get_line();
                throw std::runtime_error(boost::lexical_cast<std::string>(line) + ": Error while reading entry: " + e.what());
            }

// These are set in readConfig, once the paths have been bootstrapped.
fs::path systemConfFile;
fs::path userConfFile;

void writeConfig(Audio& m_audio, bool system) {
    xmlpp::Document doc;
    xmlpp::Node* nodeRoot = doc.create_root_node("performous");
    bool dirty = false;
    for (auto& elem: config) {
        ConfigItem& item = elem.second;
        std::string name = elem.first;
        if (item.isDefault(system)) continue; // No need to save settings with default values
        dirty = true;
        xmlpp::Element* entryNode = nodeRoot->add_child("entry");
        entryNode->set_attribute("name", name);
        std::string type = item.get_type();
        entryNode->set_attribute("type", type);
        if (name == "audio/backend") {
            int newValue = PaHostApiNameToHostApiTypeId(item.getEnumName());
            std::clog << "audio/debug: Will now change value of audio backend. New Value: " << newValue << std::endl;
            entryNode->set_attribute("value", boost::lexical_cast<std::string>(newValue));
            std::clog << "audio/info: Audio backend changed; will now restart audio subsystem." << std::endl;
            
            Audio::backendConfig().selectEnum(item.getEnumName());
            boost::thread audiokiller(boost::bind(&Audio::close, boost::ref(m_audio)));
            if (!audiokiller.timed_join(boost::posix_time::milliseconds(2500)))
                Game::getSingletonPtr()->fatalError("Audio hung for some reason.\nPlease restart Performous.");
            m_audio.restart();
            m_audio.playMusic(findFile("menu.ogg"), true); // Start music again
        }
        else if (type == "int") entryNode->set_attribute("value",std::to_string(item.i()));
        else if (type == "bool") entryNode->set_attribute("value", item.b() ? "true" : "false");
        else if (type == "float") entryNode->set_attribute("value",std::to_string(item.f()));
        else if (item.get_type() == "string") entryNode->add_child("stringvalue")->add_child_text(item.s());
        else if (item.get_type() == "string_list") {
            for (auto const& str: item.sl()) entryNode->add_child("stringvalue")->add_child_text(str);
        }
        else if (item.get_type() == "option_list") {
            //TODO: Write selected also (as attribute?)
            for (auto const& str: item.ol()) entryNode->add_child("stringvalue")->add_child_text(str);
        }
    }
    fs::path const& conf = system ? systemConfFile : userConfFile;
    std::string tmp = conf.string() + "tmp";
    try {
        create_directories(conf.parent_path());
        if (dirty) doc.write_to_file_formatted(tmp, "UTF-8");
        if (exists(conf)) remove(conf);
        if (dirty) {
            rename(tmp, conf);
            std::cerr << "Saved configuration to " << conf << std::endl;
        } else {
            std::cerr << "Using default settings, no configuration file needed." << std::endl;
        }
    } catch (...) {
        throw std::runtime_error("Unable to save " + conf.string());
    }
    if (!system) return;
    // Tell the items that we have changed the system default
    for (auto& elem: config) elem.second.makeSystem();
    // User config is no longer needed
    if (exists(userConfFile)) remove(userConfFile);
}

ConfigMenu configMenu;

void readMenuXML(xmlpp::Node* node) {
    xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(*node);
    MenuEntry me;
    me.name = getAttribute(elem, "name");
    me.shortDesc = getText(elem, "short");
    me.longDesc = getText(elem, "long");
    configMenu.push_back(me);
}

void readConfigXML(fs::path const& file, int mode) {
    if (!fs::exists(file)) {
        std::clog << "config/info: Skipping " << file << " (not found)" << std::endl;
        return;
    }
    std::clog << "config/info: Parsing " << file << std::endl;
    xmlpp::DomParser domParser(file.string());
    try {
        xmlpp::NodeSet n = domParser.get_document()->get_root_node()->find("/performous/menu/entry");
        if (!n.empty()) {
            configMenu.clear();
            std::for_each(n.begin(), n.end(), readMenuXML);
        }
        n = domParser.get_document()->get_root_node()->find("/performous/entry");
        for (xmlpp::NodeSet::const_iterator nodeit = n.begin(), end = n.end(); nodeit != end; ++nodeit) {
            xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**nodeit);
            std::string name = getAttribute(elem, "name");
            if (name.empty()) throw std::runtime_error(file.string() + " element Entry missing name attribute");
            auto it = config.find(name);
            if (mode == 0) { // Schema
                if (it != config.end()) throw std::runtime_error("Configuration schema contains the same value twice: " + name);
                config[name].update(elem, 0);
                // Add the item to menu, if not hidden
                bool hidden = false;
                try { if (getAttribute(elem, "hidden") == "true") hidden = true; } catch (XMLError&) {}
                if (!hidden) {
                    for (auto& elem: configMenu) {
                        std::string prefix = elem.name + '/';
                        if (name.substr(0, prefix.size()) == prefix) { elem.items.push_back(name); break; }
                    }
                }
            } else {
                if (it == config.end()) {
                    std::clog << "config/warning:   Entry " << name << " ignored (does not exist in config schema)." << std::endl;
                    continue;
                }
                it->second.update(elem, mode);
            }
        }
    } catch (XMLError& e) {
        int line = e.elem.get_line();
        std::string name = e.elem.get_name();
        throw std::runtime_error(file.string() + ":" + boost::lexical_cast<std::string>(line) + " element " + name + " " + e.message);
    } catch (std::exception& e) {
        throw std::runtime_error(file.string() + ":" + e.what());
    }
}

int PaHostApiNameToHostApiTypeId (const std::string& name) {
    if (name == "Auto") return 1337;
    if (name == "Windows DirectSound") return 1;
    if (name == "MME") return 2;			
    if (name == "ASIO") return 3;
    if (name == "Core Audio" || name == "CoreAudio") return 5;
    if (name == "OSS") return 7; // Not an error, stupid PortAudio.
    if (name == "ALSA") return 8;
    if (name == "Windows WDM-KS") return 11;
    if (name == "JACK Audio Connection Kit") return 12;
    if (name == "Windows WASAPI") return 13;
    throw std::runtime_error("Invalid PortAudio HostApiTypeId Specified.");
}


void readConfig() {
    // Find config schema
    fs::path schemaFile = getSchemaFilename();
    systemConfFile = getSysConfigDir() / "config.xml";
    userConfFile = getConfigDir() / "config.xml";
    readConfigXML(schemaFile, 0);  // Read schema and defaults
    readConfigXML(systemConfFile, 1);  // Update defaults with system config
    readConfigXML(userConfFile, 2);  // Read user settings
    pathInit();
    // Populate themes
    ConfigItem& ci = config["game/theme"];
    for (std::string const& theme: getThemes()) ci.addEnum(theme);
    if (ci.i() == -1) ci.selectEnum("default");  // Select the default theme if nothing is selected
}

void populateBackends (const std::list<std::string>& backendList) {
    ConfigItem& backendConfig = config["audio/backend"];
    for (std::string const& backend: backendList) backendConfig.addEnum(backend);
    static std::string selectedBackend = std::string();
    selectedBackend = backendConfig.getValue();
    backendConfig.selectEnum(selectedBackend);
}

