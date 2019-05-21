//
//  AppDelegate.swift
//  example
//
//  Created by Jianguo Wu on 2019/3/29.
//  Copyright © 2019 wujianguo. All rights reserved.
//

import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {



    func applicationDidFinishLaunching(_ aNotification: Notification) {
        ms_asnyc_start()
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }


}

