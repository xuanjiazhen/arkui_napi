// Copyright (c) 2026 Huawei Technologies Co., Ltd.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <thread>

#include "native_api.h"

// Media module namespace
namespace media {

enum class MediaFormat {
    MP4,
    AVI,
    MKV,
    MP3,
    WAV,
    FLAC,
    JPEG,
    PNG,
    GIF,
    WEBP
};

struct MediaMetadata {
    std::string title;
    std::string artist;
    std::string album;
    int duration = 0;
    int width = 0;
    int height = 0;
    int bitrate = 0;
    MediaFormat format = MediaFormat::MP4;
    std::string codec;
    std::string creationDate;
    std::string lastModified;
};

struct MediaTrack {
    enum class Type {
        AUDIO,
        VIDEO,
        SUBTITLE
    };
    Type type;
    std::string codec;
    int bitrate = 0;
    int channels = 0;
    int sampleRate = 0;
    int width = 0;
    int height = 0;
    int frameRate = 0;
    std::string language;
};

enum class PlayerState {
    IDLE,
    PREPARING,
    READY,
    PLAYING,
    PAUSED,
    COMPLETED,
    ERROR
};

struct EncoderSettings {
    MediaFormat outputFormat = MediaFormat::MP4;
    int videoBitrate = 2000;
    int audioBitrate = 128;
    int frameRate = 30;
    int width = 1920;
    int height = 1080;
    int audioSampleRate = 44100;
    int audioChannels = 2;
    std::string videoCodec = "h264";
    std::string audioCodec = "aac";
};



class MediaProcessor {
public:
    MediaProcessor() {
        std::random_device rd;
        gen_ = std::mt19937(rd());
        dist_ = std::uniform_real_distribution<>(0.0, 1.0);
    }

    MediaMetadata analyzeMedia(const std::string& filePath) {
        MediaMetadata metadata;
        metadata.title = "Sample Media";
        metadata.artist = "Unknown Artist";
        metadata.album = "Unknown Album";
        metadata.duration = 180;
        metadata.width = 1920;
        metadata.height = 1080;
        metadata.bitrate = 5000;
        metadata.format = MediaFormat::MP4;
        metadata.codec = "h264/aac";
        metadata.creationDate = "2026-01-01";
        metadata.lastModified = "2026-01-01";
        return metadata;
    }

    std::vector<MediaTrack> getMediaTracks(const std::string& filePath) {
        std::vector<MediaTrack> tracks;
        
        MediaTrack videoTrack;
        videoTrack.type = MediaTrack::Type::VIDEO;
        videoTrack.codec = "h264";
        videoTrack.bitrate = 4000;
        videoTrack.width = 1920;
        videoTrack.height = 1080;
        videoTrack.frameRate = 30;
        tracks.push_back(videoTrack);
        
        MediaTrack audioTrack;
        audioTrack.type = MediaTrack::Type::AUDIO;
        audioTrack.codec = "aac";
        audioTrack.bitrate = 128;
        audioTrack.channels = 2;
        audioTrack.sampleRate = 44100;
        tracks.push_back(audioTrack);
        
        return tracks;
    }

    bool transcodeMedia(const std::string& inputPath, const std::string& outputPath, const EncoderSettings& settings) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return true;
    }

    bool extractAudio(const std::string& videoPath, const std::string& audioPath) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return true;
    }

    bool mergeAudioVideo(const std::string& videoPath, const std::string& audioPath, const std::string& outputPath) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return true;
    }

    bool generateThumbnail(const std::string& videoPath, const std::string& thumbnailPath, int timeMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return true;
    }

    // Get media information as JSON
    std::string getMediaInfoJSON(const std::string& filePath) {
        MediaMetadata metadata = analyzeMedia(filePath);
        std::vector<MediaTrack> tracks = getMediaTracks(filePath);
        
        std::string json = "{";
        json += "\"title\":\"" + metadata.title + "\",";
        json += "\"artist\":\"" + metadata.artist + "\",";
        json += "\"album\":\"" + metadata.album + "\",";
        json += "\"duration\":" + std::to_string(metadata.duration) + ",";
        json += "\"width\":" + std::to_string(metadata.width) + ",";
        json += "\"height\":" + std::to_string(metadata.height) + ",";
        json += "\"bitrate\":" + std::to_string(metadata.bitrate) + ",";
        json += "\"format\":\"MP4\",";
        json += "\"codec\":\"" + metadata.codec + "\",";
        json += "\"creationDate\":\"" + metadata.creationDate + ",";
        json += "\"lastModified\":\"" + metadata.lastModified + ",";
        json += "\"tracks\":[";
        
        for (size_t i = 0; i < tracks.size(); ++i) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"type\":\"" + (tracks[i].type == MediaTrack::Type::VIDEO ? "video" : 
                                         tracks[i].type == MediaTrack::Type::AUDIO ? "audio" : "subtitle") + "\",";
            json += "\"codec\":\"" + tracks[i].codec + "\",";
            json += "\"bitrate\":" + std::to_string(tracks[i].bitrate) + ",";
            if (tracks[i].type == MediaTrack::Type::AUDIO) {
                json += "\"channels\":" + std::to_string(tracks[i].channels) + ",";
                json += "\"sampleRate\":" + std::to_string(tracks[i].sampleRate);
            } else if (tracks[i].type == MediaTrack::Type::VIDEO) {
                json += "\"width\":" + std::to_string(tracks[i].width) + ",";
                json += "\"height\":" + std::to_string(tracks[i].height) + ",";
                json += "\"frameRate\":" + std::to_string(tracks[i].frameRate);
            }
            json += "}";
        }
        
        json += "]";
        json += "}";
        return json;
    }



private:
    std::mt19937 gen_;
    std::uniform_real_distribution<> dist_;
};

class MediaPlayer {
public:
    MediaPlayer() : state_(PlayerState::IDLE) {}

    bool setSource(const std::string& filePath) {
        state_ = PlayerState::PREPARING;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        state_ = PlayerState::READY;
        return true;
    }

    bool play() {
        if (state_ == PlayerState::READY || state_ == PlayerState::PAUSED) {
            state_ = PlayerState::PLAYING;
            return true;
        }
        return false;
    }

    bool pause() {
        if (state_ == PlayerState::PLAYING) {
            state_ = PlayerState::PAUSED;
            return true;
        }
        return false;
    }

    bool stop() {
        if (state_ != PlayerState::IDLE && state_ != PlayerState::ERROR) {
            state_ = PlayerState::IDLE;
            return true;
        }
        return false;
    }

    bool seek(int positionMs) {
        if (state_ == PlayerState::PLAYING || state_ == PlayerState::PAUSED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return true;
        }
        return false;
    }

    bool setVolume(float volume) {
        if (volume >= 0.0 && volume <= 1.0) {
            volume_ = volume;
            return true;
        }
        return false;
    }

    PlayerState getState() const {
        return state_;
    }

    int getCurrentPosition() const {
        return 0;
    }

    int getDuration() const {
        return 180000;
    }

private:
    PlayerState state_;
    float volume_ = 1.0;
};

class MediaUtils {
public:
    static std::string formatToString(MediaFormat format) {
        switch (format) {
            case MediaFormat::MP4: return "MP4";
            case MediaFormat::AVI: return "AVI";
            case MediaFormat::MKV: return "MKV";
            case MediaFormat::MP3: return "MP3";
            case MediaFormat::WAV: return "WAV";
            case MediaFormat::FLAC: return "FLAC";
            case MediaFormat::JPEG: return "JPEG";
            case MediaFormat::PNG: return "PNG";
            case MediaFormat::GIF: return "GIF";
            case MediaFormat::WEBP: return "WEBP";
            default: return "Unknown";
        }
    }

    static MediaFormat stringToFormat(const std::string& formatStr) {
        if (formatStr == "MP4") return MediaFormat::MP4;
        if (formatStr == "AVI") return MediaFormat::AVI;
        if (formatStr == "MKV") return MediaFormat::MKV;
        if (formatStr == "MP3") return MediaFormat::MP3;
        if (formatStr == "WAV") return MediaFormat::WAV;
        if (formatStr == "FLAC") return MediaFormat::FLAC;
        if (formatStr == "JPEG" || formatStr == "jpg") return MediaFormat::JPEG;
        if (formatStr == "PNG" || formatStr == "png") return MediaFormat::PNG;
        if (formatStr == "GIF" || formatStr == "gif") return MediaFormat::GIF;
        if (formatStr == "WEBP" || formatStr == "webp") return MediaFormat::WEBP;
        return MediaFormat::MP4;
    }

    static bool validateMediaFile(const std::string& filePath) {
        return true;
    }


};

} // namespace media

// NAPI wrapper functions
namespace {

std::shared_ptr<media::MediaProcessor> g_mediaProcessor = std::make_shared<media::MediaProcessor>();
std::unordered_map<napi_env, std::shared_ptr<media::MediaPlayer>> g_players;

// NAPI function: analyzeMedia
napi_value AnalyzeMedia(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing file path argument");
        return nullptr;
    }

    char filePath[1024];
    size_t filePathLength;
    napi_get_value_string_utf8(env, args[0], filePath, sizeof(filePath), &filePathLength);

    media::MediaMetadata metadata = g_mediaProcessor->analyzeMedia(filePath);

    napi_value result;
    napi_create_object(env, &result);

    napi_value title;
    napi_create_string_utf8(env, metadata.title.c_str(), NAPI_AUTO_LENGTH, &title);
    napi_set_named_property(env, result, "title", title);

    napi_value artist;
    napi_create_string_utf8(env, metadata.artist.c_str(), NAPI_AUTO_LENGTH, &artist);
    napi_set_named_property(env, result, "artist", artist);

    napi_value album;
    napi_create_string_utf8(env, metadata.album.c_str(), NAPI_AUTO_LENGTH, &album);
    napi_set_named_property(env, result, "album", album);

    napi_value duration;
    napi_create_int32(env, metadata.duration, &duration);
    napi_set_named_property(env, result, "duration", duration);

    napi_value width;
    napi_create_int32(env, metadata.width, &width);
    napi_set_named_property(env, result, "width", width);

    napi_value height;
    napi_create_int32(env, metadata.height, &height);
    napi_set_named_property(env, result, "height", height);

    napi_value bitrate;
    napi_create_int32(env, metadata.bitrate, &bitrate);
    napi_set_named_property(env, result, "bitrate", bitrate);

    napi_value format;
    napi_create_string_utf8(env, media::MediaUtils::formatToString(metadata.format).c_str(), NAPI_AUTO_LENGTH, &format);
    napi_set_named_property(env, result, "format", format);

    napi_value codec;
    napi_create_string_utf8(env, metadata.codec.c_str(), NAPI_AUTO_LENGTH, &codec);
    napi_set_named_property(env, result, "codec", codec);

    napi_value creationDate;
    napi_create_string_utf8(env, metadata.creationDate.c_str(), NAPI_AUTO_LENGTH, &creationDate);
    napi_set_named_property(env, result, "creationDate", creationDate);

    napi_value lastModified;
    napi_create_string_utf8(env, metadata.lastModified.c_str(), NAPI_AUTO_LENGTH, &lastModified);
    napi_set_named_property(env, result, "lastModified", lastModified);

    return result;
}

// NAPI function: getMediaTracks
napi_value GetMediaTracks(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing file path argument");
        return nullptr;
    }

    char filePath[1024];
    size_t filePathLength;
    napi_get_value_string_utf8(env, args[0], filePath, sizeof(filePath), &filePathLength);

    std::vector<media::MediaTrack> tracks = g_mediaProcessor->getMediaTracks(filePath);

    napi_value result;
    napi_create_array(env, &result);

    for (size_t i = 0; i < tracks.size(); ++i) {
        napi_value track;
        napi_create_object(env, &track);

        napi_value type;
        napi_create_string_utf8(env, 
            (tracks[i].type == media::MediaTrack::Type::VIDEO ? "video" : 
             tracks[i].type == media::MediaTrack::Type::AUDIO ? "audio" : "subtitle"), 
            NAPI_AUTO_LENGTH, &type);
        napi_set_named_property(env, track, "type", type);

        napi_value codec;
        napi_create_string_utf8(env, tracks[i].codec.c_str(), NAPI_AUTO_LENGTH, &codec);
        napi_set_named_property(env, track, "codec", codec);

        napi_value bitrate;
        napi_create_int32(env, tracks[i].bitrate, &bitrate);
        napi_set_named_property(env, track, "bitrate", bitrate);

        if (tracks[i].type == media::MediaTrack::Type::AUDIO) {
            napi_value channels;
            napi_create_int32(env, tracks[i].channels, &channels);
            napi_set_named_property(env, track, "channels", channels);

            napi_value sampleRate;
            napi_create_int32(env, tracks[i].sampleRate, &sampleRate);
            napi_set_named_property(env, track, "sampleRate", sampleRate);
        } else if (tracks[i].type == media::MediaTrack::Type::VIDEO) {
            napi_value width;
            napi_create_int32(env, tracks[i].width, &width);
            napi_set_named_property(env, track, "width", width);

            napi_value height;
            napi_create_int32(env, tracks[i].height, &height);
            napi_set_named_property(env, track, "height", height);

            napi_value frameRate;
            napi_create_int32(env, tracks[i].frameRate, &frameRate);
            napi_set_named_property(env, track, "frameRate", frameRate);
        }

        napi_set_element(env, result, i, track);
    }

    return result;
}

// NAPI function: transcodeMedia
napi_value TranscodeMedia(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 3) {
        napi_throw_error(env, "BAD_ARGS", "Missing arguments");
        return nullptr;
    }

    char inputPath[1024], outputPath[1024];
    size_t inputPathLength, outputPathLength;
    napi_get_value_string_utf8(env, args[0], inputPath, sizeof(inputPath), &inputPathLength);
    napi_get_value_string_utf8(env, args[1], outputPath, sizeof(outputPath), &outputPathLength);

    media::EncoderSettings settings;
    // Parse settings from args[2]

    bool success = g_mediaProcessor->transcodeMedia(inputPath, outputPath, settings);

    napi_value result;
    napi_create_bool(env, success, &result);
    return result;
}

// NAPI function: extractAudio
napi_value ExtractAudio(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2) {
        napi_throw_error(env, "BAD_ARGS", "Missing arguments");
        return nullptr;
    }

    char videoPath[1024], audioPath[1024];
    size_t videoPathLength, audioPathLength;
    napi_get_value_string_utf8(env, args[0], videoPath, sizeof(videoPath), &videoPathLength);
    napi_get_value_string_utf8(env, args[1], audioPath, sizeof(audioPath), &audioPathLength);

    bool success = g_mediaProcessor->extractAudio(videoPath, audioPath);

    napi_value result;
    napi_create_bool(env, success, &result);
    return result;
}

// NAPI function: mergeAudioVideo
napi_value MergeAudioVideo(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 3) {
        napi_throw_error(env, "BAD_ARGS", "Missing arguments");
        return nullptr;
    }

    char videoPath[1024], audioPath[1024], outputPath[1024];
    size_t videoPathLength, audioPathLength, outputPathLength;
    napi_get_value_string_utf8(env, args[0], videoPath, sizeof(videoPath), &videoPathLength);
    napi_get_value_string_utf8(env, args[1], audioPath, sizeof(audioPath), &audioPathLength);
    napi_get_value_string_utf8(env, args[2], outputPath, sizeof(outputPath), &outputPathLength);

    bool success = g_mediaProcessor->mergeAudioVideo(videoPath, audioPath, outputPath);

    napi_value result;
    napi_create_bool(env, success, &result);
    return result;
}

// NAPI function: generateThumbnail
napi_value GenerateThumbnail(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 3) {
        napi_throw_error(env, "BAD_ARGS", "Missing arguments");
        return nullptr;
    }

    char videoPath[1024], thumbnailPath[1024];
    size_t videoPathLength, thumbnailPathLength;
    napi_get_value_string_utf8(env, args[0], videoPath, sizeof(videoPath), &videoPathLength);
    napi_get_value_string_utf8(env, args[1], thumbnailPath, sizeof(thumbnailPath), &thumbnailPathLength);

    int timeMs;
    napi_get_value_int32(env, args[2], &timeMs);

    bool success = g_mediaProcessor->generateThumbnail(videoPath, thumbnailPath, timeMs);

    napi_value result;
    napi_create_bool(env, success, &result);
    return result;
}

// NAPI function: getMediaInfoJSON
napi_value GetMediaInfoJSON(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing file path argument");
        return nullptr;
    }

    char filePath[1024];
    size_t filePathLength;
    napi_get_value_string_utf8(env, args[0], filePath, sizeof(filePath), &filePathLength);

    std::string json = g_mediaProcessor->getMediaInfoJSON(filePath);

    napi_value result;
    napi_create_string_utf8(env, json.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}



// NAPI function: createPlayer
napi_value CreatePlayer(napi_env env, napi_callback_info info) {
    auto player = std::make_shared<media::MediaPlayer>();
    g_players[env] = player;

    napi_value result;
    napi_create_object(env, &result);
    return result;
}

// NAPI function: playerSetSource
napi_value PlayerSetSource(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing file path argument");
        return nullptr;
    }

    char filePath[1024];
    size_t filePathLength;
    napi_get_value_string_utf8(env, args[0], filePath, sizeof(filePath), &filePathLength);

    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->setSource(filePath);

    napi_value result;
    napi_create_bool(env, success, &result);
    return result;
}

// NAPI function: playerPlay
napi_value PlayerPlay(napi_env env, napi_callback_info info) {
    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->play();

    napi_value result;
    napi_create_bool(env, success, &result);
    return result;
}

// NAPI function: playerPause
napi_value PlayerPause(napi_env env, napi_callback_info info) {
    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->pause();

    napi_value result;
    napi_create_bool(env, success, &result);
    return result;
}

// NAPI function: playerStop
napi_value PlayerStop(napi_env env, napi_callback_info info) {
    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->stop();

    napi_value result;
    napi_create_bool(env, success, &result);
    return result;
}

// NAPI function: playerSeek
napi_value PlayerSeek(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing position argument");
        return nullptr;
    }

    int positionMs;
    napi_get_value_int32(env, args[0], &positionMs);

    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->seek(positionMs);

    napi_value result;
    napi_create_bool(env, success, &result);
    return result;
}

// NAPI function: playerSetVolume
napi_value PlayerSetVolume(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing volume argument");
        return nullptr;
    }

    double volume;
    napi_get_value_double(env, args[0], &volume);

    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->setVolume(static_cast<float>(volume));

    napi_value result;
    napi_create_bool(env, success, &result);
    return result;
}

// NAPI function: playerGetState
napi_value PlayerGetState(napi_env env, napi_callback_info info) {
    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    media::PlayerState state = it->second->getState();
    const char* stateStr = "IDLE";
    switch (state) {
        case media::PlayerState::PREPARING: stateStr = "PREPARING";
            break;
        case media::PlayerState::READY: stateStr = "READY";
            break;
        case media::PlayerState::PLAYING: stateStr = "PLAYING";
            break;
        case media::PlayerState::PAUSED: stateStr = "PAUSED";
            break;
        case media::PlayerState::COMPLETED: stateStr = "COMPLETED";
            break;
        case media::PlayerState::ERROR: stateStr = "ERROR";
            break;
        default:
            break;
    }

    napi_value result;
    napi_create_string_utf8(env, stateStr, NAPI_AUTO_LENGTH, &result);
    return result;
}



} // namespace

// Module initialization
napi_value Init(napi_env env, napi_value exports) {
    // Media processor functions
    napi_property_descriptor mediaProcessorProps[] = {
        {"analyzeMedia", nullptr, AnalyzeMedia, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getMediaTracks", nullptr, GetMediaTracks, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"transcodeMedia", nullptr, TranscodeMedia, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"extractAudio", nullptr, ExtractAudio, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"mergeAudioVideo", nullptr, MergeAudioVideo, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"generateThumbnail", nullptr, GenerateThumbnail, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getMediaInfoJSON", nullptr, GetMediaInfoJSON, nullptr, nullptr, nullptr, napi_default, nullptr}
    };

    // Media player functions
    napi_value playerObj;
    napi_create_object(env, &playerObj);

    napi_property_descriptor playerProps[] = {
        {"create", nullptr, CreatePlayer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setSource", nullptr, PlayerSetSource, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"play", nullptr, PlayerPlay, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pause", nullptr, PlayerPause, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"stop", nullptr, PlayerStop, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"seek", nullptr, PlayerSeek, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setVolume", nullptr, PlayerSetVolume, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getState", nullptr, PlayerGetState, nullptr, nullptr, nullptr, napi_default, nullptr}
    };

    napi_define_properties(env, exports, sizeof(mediaProcessorProps) / sizeof(mediaProcessorProps[0]), mediaProcessorProps);
    napi_define_properties(env, playerObj, sizeof(playerProps) / sizeof(playerProps[0]), playerProps);
    napi_set_named_property(env, exports, "player", playerObj);

    return exports;
}

NAPI_MODULE(native_module_media, Init)