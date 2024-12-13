#include "Track.h"
#include <string>
#include <unordered_set>
#include <vector>

std::unordered_set<std::string> favorites;
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

