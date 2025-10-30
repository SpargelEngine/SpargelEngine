#include <stdio.h>
#include <unistd.h>

#import <AVFAudio/AVFAudio.h>

int main(int argc, char const* argv[]) {
    if (argc < 2) {
        return 1;
    }
    auto file = argv[1];
    auto fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    auto size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buf = (char*)malloc(size);
    fread(buf, size, 1, fp);
    fclose(fp);

    auto data = [NSData dataWithBytesNoCopy:buf length:size];
    NSError* error = nullptr;
    auto player = [[AVAudioPlayer alloc] initWithData:data error:&error];
    if (error) {
        NSLog(@"%@", error);
        return 1;
    }
    [player play];
    sleep(10);
    return 0;
}
