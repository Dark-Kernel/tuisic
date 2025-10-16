#include "../common/Track.h"
#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

std::unordered_set<std::string> favorites;
std::vector<std::string> favorite_tracks_strings;
std::vector<Track> favorite_tracks;

void addToFavorites(const std::string& song) {
    favorites.insert(song); // Add song to favorites
}

void removeFromFavorites(const std::string& song) {
    favorites.erase(song); // Remove song from favorites
}

bool isFavorite(const std::string& song) {
    return favorites.find(song) != favorites.end();
}

std::vector<Track> getFavoriteTracks() {
    return favorite_tracks;
}

std::vector<std::string> getFavoriteTracksString() {

  for (const auto &fav : favorite_tracks) {
    favorite_tracks_strings.push_back(fav.to_string());
  }
    return favorite_tracks_strings;
}


auto fetch_favorites(std::vector<Track> track_data) {
  // temp_track_data = track_data;
  // Clear previous data
  favorite_tracks_strings.clear();
  track_data.clear();

  // Limit recently played to last 10 unique tracks
  std::vector<Track> unique_favorite_tracks;
  for (const auto &track : favorite_tracks) {
    // Check if this exact track is not already in unique list
    auto it = std::find_if(unique_favorite_tracks.begin(), unique_favorite_tracks.end(),
        [&track](const Track& existing) { return existing.to_string() == track.to_string(); });
    
    if (it == unique_favorite_tracks.end()) {
      unique_favorite_tracks.push_back(track);
    }
  }

  // Keep only the last 10 tracks
  if (unique_favorite_tracks.size() > 10) {
    unique_favorite_tracks = std::vector<Track>(
      unique_favorite_tracks.end() - 10, 
      unique_favorite_tracks.end()
    );
  }

  // Populate track_data and recently_played_strings
  track_data = unique_favorite_tracks;
  for (const auto &recent : track_data) {
    favorite_tracks_strings.push_back(recent.to_string());
  }
  return favorite_tracks_strings;
}
