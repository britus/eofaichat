#include <Foundation/NSData.h>
#include <Foundation/NSString.h>
#include <Foundation/NSURL.h>
#include <AppKit/NSOpenPanel.h>
#include <appbundle.h>

extern "C" {

/**
 * Return bundle version
 */
const char* getBundleVersion() {
    NSBundle *bundle = [NSBundle mainBundle];
    if (!bundle) return "Unknown";

    NSDictionary *infoDict = [bundle infoDictionary];
    NSString *version = [infoDict objectForKey:@"CFBundleVersion"];
    return version ? [version UTF8String] : "0.0";
}

/**
 * Return bundle build number
 */
const char* getBuildNumber() {
    NSBundle *bundle = [NSBundle mainBundle];
    if (!bundle) return "Unknown";

    NSDictionary *infoDict = [bundle infoDictionary];
    NSString *build = [infoDict objectForKey:@"CFBundleShortVersionString"];
    return build ? [build UTF8String] : "0";
}

} // extern "C"
