/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

const media = require('./build/Release/native_module_media.node');

console.log('=== Media Module Test ===');

// Test 1: Get supported formats
console.log('\n1. Supported formats:');
const formats = media.getSupportedFormats();
console.log('Formats:', formats);

// Test 2: Validate media file
console.log('\n2. Validate media file:');
const isValid = media.validateMediaFile('test.mp4');
console.log('Is valid:', isValid);

// Test 3: Analyze media
console.log('\n3. Analyze media:');
try {
    const metadata = media.analyzeMedia('test.mp4');
    console.log('Metadata:', metadata);
} catch (error) {
    console.error('Error analyzing media:', error);
}

// Test 4: Get media tracks
console.log('\n4. Get media tracks:');
try {
    const tracks = media.getMediaTracks('test.mp4');
    console.log('Tracks:', tracks);
} catch (error) {
    console.error('Error getting tracks:', error);
}

// Test 5: Get media info as JSON
console.log('\n5. Get media info JSON:');
try {
    const jsonInfo = media.getMediaInfoJSON('test.mp4');
    console.log('JSON info:', JSON.parse(jsonInfo));
} catch (error) {
    console.error('Error getting JSON info:', error);
}

// Test 6: Media player
console.log('\n6. Media player tests:');
try {
    // Create player
    const player = media.player.create();
    console.log('Player created successfully');
    
    // Set source
    const setSourceResult = media.player.setSource('test.mp4');
    console.log('Set source result:', setSourceResult);
    
    // Get state
    const state = media.player.getState();
    console.log('Current state:', state);
    
    // Play
    const playResult = media.player.play();
    console.log('Play result:', playResult);
    
    // Get state after play
    const stateAfterPlay = media.player.getState();
    console.log('State after play:', stateAfterPlay);
    
    // Pause
    const pauseResult = media.player.pause();
    console.log('Pause result:', pauseResult);
    
    // Get state after pause
    const stateAfterPause = media.player.getState();
    console.log('State after pause:', stateAfterPause);
    
    // Seek
    const seekResult = media.player.seek(30000); // 30 seconds
    console.log('Seek result:', seekResult);
    
    // Set volume
    const setVolumeResult = media.player.setVolume(0.5);
    console.log('Set volume result:', setVolumeResult);
    
    // Get current position
    const position = media.player.getCurrentPosition();
    console.log('Current position:', position);
    
    // Get duration
    const duration = media.player.getDuration();
    console.log('Duration:', duration);
    
    // Stop
    const stopResult = media.player.stop();
    console.log('Stop result:', stopResult);
    
    // Get state after stop
    const stateAfterStop = media.player.getState();
    console.log('State after stop:', stateAfterStop);
    
} catch (error) {
    console.error('Error with media player:', error);
}

// Test 7: Transcode media
console.log('\n7. Transcode media:');
try {
    const transcodeResult = media.transcodeMedia('input.mp4', 'output.mp4', {});
    console.log('Transcode result:', transcodeResult);
} catch (error) {
    console.error('Error transcoding media:', error);
}

// Test 8: Extract audio
console.log('\n8. Extract audio:');
try {
    const extractResult = media.extractAudio('video.mp4', 'audio.mp3');
    console.log('Extract audio result:', extractResult);
} catch (error) {
    console.error('Error extracting audio:', error);
}

// Test 9: Merge audio and video
console.log('\n9. Merge audio and video:');
try {
    const mergeResult = media.mergeAudioVideo('video.mp4', 'audio.mp3', 'output.mp4');
    console.log('Merge result:', mergeResult);
} catch (error) {
    console.error('Error merging audio and video:', error);
}

// Test 10: Generate thumbnail
console.log('\n10. Generate thumbnail:');
try {
    const thumbnailResult = media.generateThumbnail('video.mp4', 'thumbnail.jpg', 10000); // 10 seconds
    console.log('Generate thumbnail result:', thumbnailResult);
} catch (error) {
    console.error('Error generating thumbnail:', error);
}

console.log('\n=== Test completed ===');