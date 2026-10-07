// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/MacNativeNotifications.h"
#include <QPointer>
#include <QUuid>
#import <UserNotifications/UserNotifications.h>

using sentinel::desktop::MacNativeNotifications;
@interface SentinelNotificationDelegate : NSObject <UNUserNotificationCenterDelegate> {
  @public
    QPointer<MacNativeNotifications> owner;
}
@end
@implementation SentinelNotificationDelegate
- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completion {
    Q_UNUSED(center);
    Q_UNUSED(notification);
    completion(UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList);
}
- (void)userNotificationCenter:(UNUserNotificationCenter *)center
    didReceiveNotificationResponse:(UNNotificationResponse *)response
             withCompletionHandler:(void (^)(void))completion {
    if ([response.actionIdentifier isEqualToString:UNNotificationDefaultActionIdentifier]) {
        const auto context = owner;
        const NSDictionary *info = response.notification.request.content.userInfo;
        const QString session = QString::fromUtf8([info[@"session"] UTF8String]);
        const QString page = QString::fromUtf8([info[@"page"] UTF8String]);
        if (context)
            QMetaObject::invokeMethod(
                context,
                [context, session, page] {
                    if (context)
                        emit context->clicked(session, page);
                },
                Qt::QueuedConnection);
    }
    [center
        removeDeliveredNotificationsWithIdentifiers:@[ response.notification.request.identifier ]];
    completion();
}
@end

namespace sentinel::desktop {
namespace {
NSString *nativeString(const QString &value) {
    return [NSString stringWithUTF8String:value.toUtf8().constData()];
}
QString status(UNNotificationSettings *settings) {
    switch (settings.authorizationStatus) {
    case UNAuthorizationStatusNotDetermined:
        return "Not requested";
    case UNAuthorizationStatusDenied:
        return "Denied in macOS Settings";
    case UNAuthorizationStatusAuthorized:
        return settings.alertSetting == UNNotificationSettingEnabled
                   ? "Authorized"
                   : "Authorized; alerts disabled in macOS Settings";
    case UNAuthorizationStatusProvisional:
        return "Provisional";
    default:
        return "Unavailable";
    }
}
} // namespace
MacNativeNotifications::MacNativeNotifications(QObject *parent) : QObject(parent) {
    auto *delegate = [[SentinelNotificationDelegate alloc] init];
    delegate->owner = this;
    delegate_ = delegate;
    UNUserNotificationCenter.currentNotificationCenter.delegate = delegate;
    refreshAuthorization();
}
MacNativeNotifications::~MacNativeNotifications() {
    auto *center = UNUserNotificationCenter.currentNotificationCenter;
    auto *ids = [NSMutableArray array];
    for (const auto &id : identifiers_)
        [ids addObject:nativeString(id)];
    [center removePendingNotificationRequestsWithIdentifiers:ids];
    [center removeDeliveredNotificationsWithIdentifiers:ids];
    auto *delegate = static_cast<SentinelNotificationDelegate *>(delegate_);
    delegate->owner.clear();
    if (center.delegate == delegate)
        center.delegate = nil;
    [delegate release];
}
void MacNativeNotifications::refreshAuthorization() {
    const QPointer<MacNativeNotifications> context(this);
    [UNUserNotificationCenter.currentNotificationCenter
        getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings *settings) {
          const auto value = status(settings);
          if (context)
              QMetaObject::invokeMethod(
                  context,
                  [context, value] {
                      if (context)
                          emit context->statusChanged(value);
                  },
                  Qt::QueuedConnection);
        }];
}
void MacNativeNotifications::deliver(const QString &titleArg, const QString &bodyArg,
                                     const QString &sessionArg, const QString &pageArg) {
    // Objective-C blocks must own values, not capture C++ reference parameters.
    const QString title(titleArg), body(bodyArg), session(sessionArg), page(pageArg);
    const QPointer<MacNativeNotifications> context(this);
    [UNUserNotificationCenter.currentNotificationCenter
        getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings *settings) {
          const auto state = settings.authorizationStatus;
          if (context)
              QMetaObject::invokeMethod(
                  context,
                  [context, state, title, body, session, page] {
                      if (!context)
                          return;
                      context->refreshAuthorization();
                      if (state == UNAuthorizationStatusDenied)
                          return;
                      if (state == UNAuthorizationStatusNotDetermined) {
                          if (context->authorizationRequested_)
                              return;
                          context->authorizationRequested_ = true;
                          [UNUserNotificationCenter.currentNotificationCenter
                              requestAuthorizationWithOptions:UNAuthorizationOptionAlert
                                            completionHandler:^(BOOL granted, NSError *error) {
                                              const auto failure =
                                                  error
                                                      ? QString::fromUtf8(error.domain.UTF8String) +
                                                            ":" + QString::number(error.code)
                                                      : QString{};
                                              if (context)
                                                  QMetaObject::invokeMethod(
                                                      context,
                                                      [context, granted, title, body, session, page,
                                                       failure] {
                                                          if (!context)
                                                              return;
                                                          if (!failure.isEmpty()) {
                                                              emit context->statusChanged(
                                                                  "Authorization request failed (" +
                                                                  failure + ")");
                                                              return;
                                                          }
                                                          context->refreshAuthorization();
                                                          if (granted)
                                                              context->deliver(title, body, session,
                                                                               page);
                                                      },
                                                      Qt::QueuedConnection);
                                            }];
                          return;
                      }
                      auto *content = [[[UNMutableNotificationContent alloc] init] autorelease];
                      content.title = nativeString(title);
                      content.body = nativeString(body);
                      content.userInfo =
                          @{@"session" : nativeString(session), @"page" : nativeString(page)};
                      const auto id =
                          "sentinel-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
                      context->identifiers_.append(id);
                      // Bound retained notifications; never remove another client's notifications.
                      if (context->identifiers_.size() > 64) {
                          const auto oldest = context->identifiers_.takeFirst();
                          [UNUserNotificationCenter.currentNotificationCenter
                              removeDeliveredNotificationsWithIdentifiers:@[ nativeString(
                                                                              oldest) ]];
                      }
                      auto *request = [UNNotificationRequest requestWithIdentifier:nativeString(id)
                                                                           content:content
                                                                           trigger:nil];
                      [UNUserNotificationCenter.currentNotificationCenter
                          addNotificationRequest:request
                           withCompletionHandler:^(NSError *error) {
                             if (error && context)
                                 QMetaObject::invokeMethod(
                                     context,
                                     [context] {
                                         if (context)
                                             emit context->statusChanged(
                                                 "Delivery failed; check macOS Settings");
                                     },
                                     Qt::QueuedConnection);
                           }];
                  },
                  Qt::QueuedConnection);
        }];
}
} // namespace sentinel::desktop
