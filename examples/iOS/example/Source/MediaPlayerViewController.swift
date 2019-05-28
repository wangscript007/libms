//
//  MediaPlayerViewController.swift
//  example
//
//  Created by Jianguo Wu on 2019/5/28.
//  Copyright © 2019 wujianguo. All rights reserved.
//

import UIKit
import AVKit

class MediaPlayerViewController: UIViewController {

    var items = [MediaItem]()
    var index = 0
    
    var playerViewController: AVPlayerViewController!
    
    override func viewDidLoad() {
        super.viewDidLoad()

        updateToolBarStatus()
    }
    
    func updateToolBarStatus() {
        previousBarButton.isEnabled = index > 0
        nextBarButton.isEnabled = index < items.count - 1
    }
    
    func play() {
        let item = items[index]
        title = item.name
        let url = MediaServer.generateURL(url: item.url)
        playerViewController.player = AVPlayer(url: url)
        playerViewController.player?.play()
    }
    
    // MARK: - Actions
    
    @IBOutlet weak var previousBarButton: UIBarButtonItem!
    
    @IBOutlet weak var nextBarButton: UIBarButtonItem!
    
    
    @IBAction func didClickPreviousBarButton(_ sender: UIBarButtonItem) {
        guard index > 0 else {
            updateToolBarStatus()
            return
        }
        index -= 1
        play()
        updateToolBarStatus()
    }
    
    @IBAction func didClickNextBarButton(_ sender: UIBarButtonItem) {
        guard index < items.count - 1 else {
            updateToolBarStatus()
            return
        }
        index += 1
        play()
        updateToolBarStatus()
    }
    
    
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        if let vc = segue.destination as? AVPlayerViewController {
            playerViewController = vc
            play()
        }
    }
    
    
}
