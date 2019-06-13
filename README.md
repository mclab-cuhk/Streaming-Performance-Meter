Streaming Performance Meter

We have a high resolution bluray movie database of over 700 licensed popular titles.

Each of them are bitrate scanned and data recorded down to video frame level.

This is a project to re-construct a placeholder movie with exactly the same bitrate profile as recorded from any of
the bluray movie titles with barcodes embedded frame information to allow for QoE evaluation during playback

The Streaming Performance Meter part is the video processor to decode the recorded playback of the virtual movie with barcode data of original frame information to yield the statistical data for QOE calculation

This repositiory contains the necessary source code files to build the Streaming Performance Meter:

ffmpeg-3.2.4/      contains the modified source code tree of open source ffmpeg video processor
barcode-0.99/      contains the original source code tree of open source dependent barcode library used in the ffmpeg video processor
Zbar-0.10/         contains the original source code tree of open source dependent barcode library used in the ffmpeg video processor
