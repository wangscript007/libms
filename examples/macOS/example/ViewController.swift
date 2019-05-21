//
//  ViewController.swift
//  example
//
//  Created by Jianguo Wu on 2019/3/29.
//  Copyright © 2019 wujianguo. All rights reserved.
//

import Cocoa

class MediaCollectionViewItem: NSCollectionViewItem {
    
    static var identifier = NSUserInterfaceItemIdentifier(rawValue: "MediaCollectionViewItemIdentifier")
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
    }
    
}

class ViewController: NSViewController, NSCollectionViewDataSource {

    
    @IBOutlet weak var collectionView: NSCollectionView!
    
    override func viewDidLoad() {
        super.viewDidLoad()

        setupCollectionView()
    }
    
    func setupCollectionView() {
        collectionView.register(MediaCollectionViewItem.self, forItemWithIdentifier: MediaCollectionViewItem.identifier)

        let flowLayout = NSCollectionViewFlowLayout()
        flowLayout.itemSize = NSSize(width: 100, height: 100)
        flowLayout.minimumInteritemSpacing = 20
        flowLayout.minimumLineSpacing = 20
        collectionView.collectionViewLayout = flowLayout
        view.wantsLayer = true
        
        collectionView.dataSource = self
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    
    func numberOfSections(in collectionView: NSCollectionView) -> Int {
        return 0
    }

    func collectionView(_ collectionView: NSCollectionView, numberOfItemsInSection section: Int) -> Int {
        return 100
    }
    
    func collectionView(_ collectionView: NSCollectionView, itemForRepresentedObjectAt indexPath: IndexPath) -> NSCollectionViewItem {
        let item = collectionView.makeItem(withIdentifier: MediaCollectionViewItem.identifier, for: indexPath)
        
        return item
    }
}

