#include "PDSecurityScopedBookmarks.h"

#ifdef OS_MAC
#import <Foundation/Foundation.h>

static bool PDCStringHasContents(const char* str)
{
  return str && str[0] != '\0';
}

static NSMutableDictionary<NSString*, NSURL*>* PDSecurityScopedURLStore()
{
  static NSMutableDictionary<NSString*, NSURL*>* sSecurityScopedURLs = [[NSMutableDictionary alloc] init];
  return sSecurityScopedURLs;
}

bool CreateSecurityScopedBookmark(const char* path, WDL_String& bookmark)
{
  bookmark.Set("");

  if (!PDCStringHasContents(path))
    return false;

  @autoreleasepool
  {
    NSString* pPath = [NSString stringWithCString:path encoding:NSUTF8StringEncoding];
    if (!pPath)
      return false;

    NSURL* url = [NSURL fileURLWithPath:pPath isDirectory:YES];
    NSError* error = nil;
    NSData* data = [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
                includingResourceValuesForKeys:nil
                                 relativeToURL:nil
                                         error:&error];

    if (!data)
      return false;

    NSString* encoded = [data base64EncodedStringWithOptions:0];
    if (!encoded)
      return false;

    bookmark.Set([encoded UTF8String]);
    return true;
  }
}

bool StartAccessingSecurityScopedBookmark(const char* bookmark, WDL_String& resolvedPath)
{
  resolvedPath.Set("");

  if (!PDCStringHasContents(bookmark))
    return false;

  @autoreleasepool
  {
    NSString* encoded = [NSString stringWithCString:bookmark encoding:NSUTF8StringEncoding];
    if (!encoded)
      return false;

    NSData* data = [[NSData alloc] initWithBase64EncodedString:encoded options:0];
#if !__has_feature(objc_arc)
    [data autorelease];
#endif
    if (!data)
      return false;

    BOOL stale = NO;
    NSError* error = nil;
    NSURL* url = [NSURL URLByResolvingBookmarkData:data
                                           options:NSURLBookmarkResolutionWithSecurityScope
                                     relativeToURL:nil
                               bookmarkDataIsStale:&stale
                                             error:&error];

    if (!url)
      return false;

    NSString* pPath = [url path];
    if (!pPath)
      return false;

    resolvedPath.Set([pPath UTF8String]);

    if ([url startAccessingSecurityScopedResource])
      [PDSecurityScopedURLStore() setObject:url forKey:pPath];

    return true;
  }
}

void StopAccessingSecurityScopedBookmarks()
{
  @autoreleasepool
  {
    for (NSURL* url in [PDSecurityScopedURLStore() allValues])
      [url stopAccessingSecurityScopedResource];

    [PDSecurityScopedURLStore() removeAllObjects];
  }
}
#endif
