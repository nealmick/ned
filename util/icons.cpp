#include "icons.h"
#include "settings.h"

#include <GL/glew.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

#define NANOSVG_IMPLEMENTATION
#include "lib/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "lib/nanosvgrast.h"

namespace fs = std::filesystem;

namespace {

// SVG filenames under resources/icons/. Key in the map is the stem (before '.').
const std::vector<std::string> ICON_FILES = {
	"tf.svg",
	"terminal.svg",
	"edit-hover.svg",
	"edit.svg",
	"terminal-hover.svg",
	"cshtml.svg",
	"green-dot.svg",
	"rs.svg",
	"cmake.svg",
	"maximize-mac.svg",
	"close-mac.svg",
	"minimize-mac.svg",
	"maximize-mac-hover.svg",
	"close-mac-hover.svg",
	"minimize-mac-hover.svg",
	"clangd.svg",
	"ini.svg",
	"clang-format.svg",
	"rb.svg",
	"Dockerfile.svg",
	"sh.svg",
	"cs.svg",
	"csproj.svg",
	"md.svg",
	"tsx.svg",
	"jsx.svg",
	"ts.svg",
	"brain.svg",
	"close.svg",
	"gear.svg",
	"gear-hover.svg",
	"py.svg",
	"h.svg",
	"hpp.svg",
	"gitignore.svg",
	"gitmodule.svg",
	"js.svg",
	"R.svg",
	"apple.svg",
	"argdown.svg",
	"asm.svg",
	"audio.svg",
	"babel.svg",
	"bazel.svg",
	"bicep.svg",
	"bower.svg",
	"bsl.svg",
	"c-sharp.svg",
	"c.svg",
	"cake.svg",
	"cake_php.svg",
	"checkbox-unchecked.svg",
	"checkbox.svg",
	"cjsx.svg",
	"clock.svg",
	"clojure.svg",
	"code-climate.svg",
	"code-search.svg",
	"coffee.svg",
	"coffee_erb.svg",
	"coldfusion.svg",
	"config.svg",
	"cpp.svg",
	"crystal.svg",
	"crystal_embedded.svg",
	"css.svg",
	"csv.svg",
	"cu.svg",
	"d.svg",
	"dart.svg",
	"db.svg",
	"default.svg",
	"deprecation-cop.svg",
	"docker.svg",
	"editorconfig.svg",
	"ejs.svg",
	"elixir.svg",
	"elixir_script.svg",
	"elm.svg",
	"error.svg",
	"eslint.svg",
	"ethereum.svg",
	"f-sharp.svg",
	"favicon.svg",
	"firebase.svg",
	"firefox.svg",
	"font.svg",
	"git.svg",
	"git_folder.svg",
	"folder.svg",
	"folder-open.svg",
	"git_ignore.svg",
	"github.svg",
	"gitlab.svg",
	"go.svg",
	"go2.svg",
	"godot.svg",
	"gradle.svg",
	"grails.svg",
	"graphql.svg",
	"grunt.svg",
	"gulp.svg",
	"hacklang.svg",
	"haml.svg",
	"happenings.svg",
	"haskell.svg",
	"haxe.svg",
	"heroku.svg",
	"hex.svg",
	"html.svg",
	"html_erb.svg",
	"ignored.svg",
	"illustrator.svg",
	"image.svg",
	"info.svg",
	"ionic.svg",
	"jade.svg",
	"java.svg",
	"javascript.svg",
	"jenkins.svg",
	"jinja.svg",
	"js_erb.svg",
	"json.svg",
	"julia.svg",
	"karma.svg",
	"kotlin.svg",
	"less.svg",
	"license.svg",
	"liquid.svg",
	"livescript.svg",
	"lock.svg",
	"lua.svg",
	"makefile.svg",
	"markdown.svg",
	"maven.svg",
	"mdo.svg",
	"mustache.svg",
	"new-file.svg",
	"nim.svg",
	"notebook.svg",
	"npm.svg",
	"npm_ignored.svg",
	"nunjucks.svg",
	"ocaml.svg",
	"odata.svg",
	"pddl.svg",
	"pdf.svg",
	"perl.svg",
	"photoshop.svg",
	"php.svg",
	"pipeline.svg",
	"plan.svg",
	"platformio.svg",
	"powershell.svg",
	"prisma.svg",
	"project.svg",
	"prolog.svg",
	"pug.svg",
	"puppet.svg",
	"purescript.svg",
	"python.svg",
	"rails.svg",
	"react.svg",
	"reasonml.svg",
	"rescript.svg",
	"rollup.svg",
	"ruby.svg",
	"rust.svg",
	"salesforce.svg",
	"sass.svg",
	"sbt.svg",
	"scala.svg",
	"search.svg",
	"settings.svg",
	"shell.svg",
	"slim.svg",
	"smarty.svg",
	"spring.svg",
	"stylelint.svg",
	"stylus.svg",
	"sublime.svg",
	"svelte.svg",
	"svg.svg",
	"png.svg",
	"swift.svg",
	"terraform.svg",
	"tex.svg",
	"time-cop.svg",
	"todo.svg",
	"tsconfig.svg",
	"twig.svg",
	"typescript.svg",
	"vala.svg",
	"video.svg",
	"vue.svg",
	"wasm.svg",
	"wat.svg",
	"webpack.svg",
	"wgt.svg",
	"windows.svg",
	"word.svg",
	"xls.svg",
	"xml.svg",
	"yarn.svg",
	"yml.svg",
	"zig.svg",
	"zip.svg",
};

std::string resolveIconPath(const std::string &iconFile)
{
	// Prefer app resource root (works when cwd is not the package root — Windows
	// shortcuts, Linux /usr/share layout). Fall back to cwd-relative for dev.
	const fs::path roots[] = {
		fs::path(Settings::getAppResourcesPath()) / "resources" / "icons",
		fs::path("resources") / "icons",
#ifndef __APPLE__
		fs::path("/usr/share/Ned/resources/icons"),
#endif
	};
	for (const fs::path &dir : roots)
	{
		const fs::path full = dir / iconFile;
		if (fs::exists(full))
			return full.string();
	}
	return {};
}

} // namespace

void Icons::load()
{
	textures.clear();
	for (const auto &file : ICON_FILES)
		loadSvg(file);
	if (textures.find("default") == textures.end())
		createFallback();
}

uint32_t Icons::createTexture(const unsigned char *pixels, int width, int height)
{
	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	return static_cast<uint32_t>(texture);
}

bool Icons::loadSvg(const std::string &iconFile)
{
	const std::string path = resolveIconPath(iconFile);
	if (path.empty())
		return false;

	NSVGimage *image = nsvgParseFromFile(path.c_str(), "px", SVG_DPI);
	if (!image)
	{
		std::cerr << "Error loading SVG: " << path << std::endl;
		return false;
	}

	NSVGrasterizer *rast = nsvgCreateRasterizer();
	if (!rast)
	{
		nsvgDelete(image);
		return false;
	}

	auto pixels = std::make_unique<unsigned char[]>(ICON_SIZE * ICON_SIZE * 4);
	const float scale = (image->width > 0.0f) ? (ICON_SIZE / image->width) : 1.0f;
	nsvgRasterize(
		rast, image, 0, 0, scale, pixels.get(), ICON_SIZE, ICON_SIZE, ICON_SIZE * 4);

	const uint32_t texture = createTexture(pixels.get(), ICON_SIZE, ICON_SIZE);
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);

	if (!texture)
		return false;

	const std::string key = iconFile.substr(0, iconFile.find('.'));
	textures[key] = static_cast<ImTextureID>(static_cast<uintptr_t>(texture));
	return true;
}

void Icons::createFallback()
{
	const unsigned char pixels[] = {255, 255, 255, 255, 0, 0, 0, 255};
	const uint32_t texture = createTexture(pixels, 2, 1);
	textures["default"] = static_cast<ImTextureID>(static_cast<uintptr_t>(texture));
}

ImTextureID Icons::get(const std::string &name) const
{
	auto it = textures.find(name);
	if (it != textures.end())
		return it->second;
	auto def = textures.find("default");
	return (def != textures.end()) ? def->second : ImTextureID{};
}

ImTextureID Icons::getForFile(const std::string &filename) const
{
	const std::string name = fs::path(filename).filename().string();

	if (name == "CMakeLists.txt" || name == "cmake")
		return get("cmake");
	if (name == ".clangd" || name == ".clang-format")
		return get("clangd");
	if (name == "Dockerfile")
		return get("Dockerfile");
	if (name == ".gitignore")
		return get("gitignore");
	if (name == ".gitmodules")
		return get("gitmodule");

	std::string ext = fs::path(filename).extension().string();
	if (!ext.empty() && ext[0] == '.')
		ext.erase(0, 1);

	return get(ext.empty() ? "default" : ext);
}
