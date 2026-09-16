#include "local_music.hpp"

#include "../common/paths.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>

namespace {

bool is_audio_file(const std::filesystem::path &path) {
  static const std::set<std::string> extensions = {
      ".aac", ".flac", ".m4a", ".mp3", ".mp4", ".ogg", ".opus", ".wav",
      ".webm"};

  std::string extension = path.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return extensions.find(extension) != extensions.end();
}

std::string display_name(const std::filesystem::path &path) {
  std::string name = path.stem().string();
  std::replace(name.begin(), name.end(), '_', ' ');
  return name.empty() ? path.filename().string() : name;
}

} // namespace

std::vector<Track> discover_local_tracks(const std::string &music_root) {
  namespace fs = std::filesystem;

  const fs::path root =
      music_root.empty() ? fs::path(paths::get_music_dir()) : fs::path(music_root);
  std::vector<fs::path> search_roots;

  // Keep the downloaded/organised locations requested by the user separate
  // from unrelated files that may also live directly under ~/Music.
  for (const char *directory : {"mp4s", "mp3s"}) {
    const fs::path candidate = root / directory;
    if (fs::is_directory(candidate)) {
      search_roots.push_back(candidate);
    }
  }
  if (search_roots.empty() && fs::is_directory(root)) {
    search_roots.push_back(root);
  }

  std::vector<Track> tracks;
  std::set<std::string> seen_paths;
  for (const fs::path &search_root : search_roots) {
    std::error_code error;
    fs::recursive_directory_iterator iterator(
        search_root, fs::directory_options::skip_permission_denied, error);
    const fs::recursive_directory_iterator end;
    for (; iterator != end; iterator.increment(error)) {
      if (error) {
        error.clear();
        continue;
      }
      if (!iterator->is_regular_file(error) || error ||
          !is_audio_file(iterator->path())) {
        error.clear();
        continue;
      }

      const fs::path absolute_path = fs::absolute(iterator->path(), error);
      if (error) {
        error.clear();
        continue;
      }
      const std::string url = absolute_path.lexically_normal().string();
      if (!seen_paths.insert(url).second) {
        continue;
      }

      Track track;
      track.name = display_name(absolute_path);
      track.artist = "Local file";
      track.url = url;
      track.source = "local";
      tracks.push_back(std::move(track));
    }
  }

  std::sort(tracks.begin(), tracks.end(),
            [](const Track &left, const Track &right) {
              return left.name < right.name;
            });
  return tracks;
}
