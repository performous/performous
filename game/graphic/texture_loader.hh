#pragma once

#include "bitmap.hh"
#include "texture_reference.hh"

#include <functional>

struct TextureLoaderScopedKeeper {
	TextureLoaderScopedKeeper();
	~TextureLoaderScopedKeeper();
};

struct TextureLoadJob {
	using ApplyFunc = std::function<void(Bitmap& bitmap)>;
	fs::path name;
	ApplyFunc apply;
	Bitmap bitmap;
	bool done = false;

	TextureLoadJob() = default;
	TextureLoadJob(fs::path const& name, ApplyFunc const& apply) 
		: name(name), apply(apply) {
	}
};

using TextureLoadingId = size_t;

TextureLoadingId loadAsync(TextureLoadJob const&);

template <typename T> TextureLoadingId loadTexture(T* target, fs::path const& path) {
	// Temporarily add 1x1 pixel black texture
	Bitmap bitmap;

	bitmap.filepath = path;
	bitmap.fmt = pix::Format::RGB;
	bitmap.resize(1, 1);
	
	target->load(bitmap, false);

	return loadAsync(TextureLoadJob(path, [target](Bitmap& bitmap) {
		target->load(bitmap, false);
	}));
}

void abortTextureLoading(TextureLoadingId);

struct TextureReferenceLoader {
	TextureReferenceLoader(TextureReferencePtr textureReference) : m_textureReference(textureReference) {
	}

	void load(Bitmap const& bitmap, bool isText);

private:
	TextureReferencePtr m_textureReference;
};
