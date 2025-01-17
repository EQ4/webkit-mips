/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "WKPagePrivateMac.h"

#import "PageLoadStateObserver.h"
#import "WKAPICast.h"
#import "WKNSURLExtras.h"
#import "WebPageGroup.h"
#import "WebPageProxy.h"
#import "WebPreferences.h"
#import "WebProcessPool.h"

using namespace WebKit;

@interface WKObservablePageState : NSObject <_WKObservablePageState> {
    RefPtr<WebPageProxy> _page;
    std::unique_ptr<PageLoadStateObserver> _observer;
}

@end

@implementation WKObservablePageState

- (id)initWithPage:(RefPtr<WebPageProxy>&&)page
{
    if (!(self = [super init]))
        return nil;

    _page = WTF::move(page);
    _observer = std::make_unique<PageLoadStateObserver>(self, @"URL");
    _page->pageLoadState().addObserver(*_observer);

    return self;
}

- (void)dealloc
{
    _page->pageLoadState().removeObserver(*_observer);

    [super dealloc];
}

- (BOOL)isLoading
{
    return _page->pageLoadState().isLoading();
}

- (NSString *)title
{
    return _page->pageLoadState().title();
}

- (NSURL *)URL
{
    return [NSURL _web_URLWithWTFString:_page->pageLoadState().activeURL()];
}

- (BOOL)hasOnlySecureContent
{
    return _page->pageLoadState().hasOnlySecureContent();
}

- (double)estimatedProgress
{
    return _page->estimatedProgress();
}

@end

id <_WKObservablePageState> WKPageCreateObservableState(WKPageRef pageRef)
{
    return [[WKObservablePageState alloc] initWithPage:toImpl(pageRef)];
}

pid_t WKPageGetProcessIdentifier(WKPageRef pageRef)
{
    return toImpl(pageRef)->processIdentifier();
}

bool WKPageIsURLKnownHSTSHost(WKPageRef page, WKURLRef url)
{
    WebPageProxy* webPageProxy = toImpl(page);
    bool privateBrowsingEnabled = webPageProxy->pageGroup().preferences().privateBrowsingEnabled();

    return webPageProxy->process().processPool().isURLKnownHSTSHost(toImpl(url)->string(), privateBrowsingEnabled);
}
