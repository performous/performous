#include <gtest/gtest.h>

#include "game/fixednotegraphscaler.hh"

#include "game/notes.hh"
#include "game/dynamicnotegraphscaler.hh"
#include "game/fixednotegraphscaler.hh"

namespace {
    Note make(int note, std::string const& text, double begin = 0, double end = 1, Note::Type type = Note::NORMAL) {
        auto result = Note();
        
        result.note = note;
        result.syllable = text;
        result.begin = begin;
        result.end = end;
        result.type = type;
        
        return result;
    }
    
    TEST(UnitTest_FixedNoteGraphScaler, calculate_1_time_0) {
        auto vocal = VocalTrack("Songname");
        
        vocal.noteMin = 0;
        vocal.noteMax = 0;
        vocal.notes.emplace_back(make(0, "A"));
        
        auto scaler = FixedNoteGraphScaler();
        
        scaler.initialize(vocal);
        
        const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 0);
        
        EXPECT_EQ(0, dimension.min1);
        EXPECT_EQ(0, dimension.max1);
        EXPECT_EQ(0, dimension.min2);
        EXPECT_EQ(0, dimension.max2);
    }
    
    TEST(UnitTest_FixedNoteGraphScaler, calculate_1_time_1) {
        auto vocal = VocalTrack("Songname");
        
        vocal.noteMin = 0;
        vocal.noteMax = 0;
        vocal.notes.emplace_back(make(0, "A"));
        
        auto scaler = FixedNoteGraphScaler();
        
        scaler.initialize(vocal);
        
        const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 1);
        
        EXPECT_EQ(0, dimension.min1);
        EXPECT_EQ(0, dimension.max1);
        EXPECT_EQ(0, dimension.min2);
        EXPECT_EQ(0, dimension.max2);
    }
    
    TEST(UnitTest_FixedNoteGraphScaler, calculate_3_time_0) {
        auto vocal = VocalTrack("Songname");
        
        vocal.noteMin = 0;
        vocal.noteMax = 0;
        vocal.notes.emplace_back(make(0, "A"));
        vocal.notes.emplace_back(make(1, "A", 1, 2));
        vocal.notes.emplace_back(make(3, "A", 2, 3));
        
        auto scaler = FixedNoteGraphScaler();
        
        scaler.initialize(vocal);
        
        const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 0);
        
        EXPECT_EQ(0, dimension.min1);
        EXPECT_EQ(3, dimension.max1);
        EXPECT_EQ(0, dimension.min2);
        EXPECT_EQ(3, dimension.max2);
    }
    
    TEST(UnitTest_FixedNoteGraphScaler, calculate_3_time_1) {
        auto vocal = VocalTrack("Songname");
        
        vocal.noteMin = 0;
        vocal.noteMax = 0;
        vocal.notes.emplace_back(make(0, "A"));
        vocal.notes.emplace_back(make(1, "A", 1, 2));
        vocal.notes.emplace_back(make(3, "A", 2, 3));
        
        auto scaler = FixedNoteGraphScaler();
        
        scaler.initialize(vocal);
        
        const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 1);
        
        EXPECT_EQ(0, dimension.min1);
        EXPECT_EQ(3, dimension.max1);
        EXPECT_EQ(0, dimension.min2);
        EXPECT_EQ(3, dimension.max2);
    }
    
    TEST(UnitTest_FixedNoteGraphScaler, calculate_vocal_min) {
        auto vocal = VocalTrack("Songname");
        
        vocal.noteMin = 0;
        vocal.noteMax = 2;
        vocal.notes.emplace_back(make(1, "A"));
        vocal.notes.emplace_back(make(2, "A", 1, 2));
        vocal.notes.emplace_back(make(3, "A", 2, 3));
        
        auto scaler = FixedNoteGraphScaler();
        
        scaler.initialize(vocal);
        
        const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 1);
        
        EXPECT_EQ(1, dimension.min1);
        EXPECT_EQ(3, dimension.max1);
        EXPECT_EQ(1, dimension.min2);
        EXPECT_EQ(3, dimension.max2);
    }
    
    TEST(UnitTest_FixedNoteGraphScaler, calculate_vocal_max) {
        auto vocal = VocalTrack("Songname");
        
        vocal.noteMin = 2;
        vocal.noteMax = 5;
        vocal.notes.emplace_back(make(0, "A"));
        vocal.notes.emplace_back(make(2, "A", 1, 2));
        vocal.notes.emplace_back(make(3, "A", 2, 3));
        
        auto scaler = FixedNoteGraphScaler();
        
        scaler.initialize(vocal);
        
        const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 1);
        
        EXPECT_EQ(0, dimension.min1);
        EXPECT_EQ(3, dimension.max1);
        EXPECT_EQ(0, dimension.min2);
        EXPECT_EQ(3, dimension.max2);
    }
    
    TEST(UnitTest_FixedNoteGraphScaler, calculate_ignore_SLEEP) {
        auto vocal = VocalTrack("Songname");
        
        vocal.noteMin = 0;
        vocal.noteMax = 3;
        vocal.notes.emplace_back(make(0, "A", 0, 1));
        vocal.notes.emplace_back(make(3, "A", 1, 2, Note::SLEEP));
        vocal.notes.emplace_back(make(1, "A", 2, 3));
        
        auto scaler = FixedNoteGraphScaler();
        
        scaler.initialize(vocal);
        
        const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 0);
        
        EXPECT_EQ(0, dimension.min1);
        EXPECT_EQ(1, dimension.max1);
        EXPECT_EQ(0, dimension.min2);
        EXPECT_EQ(1, dimension.max2);
    }
} 
 
 
