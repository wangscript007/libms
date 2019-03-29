//
//  MediaCollectionViewCell.swift
//  example
//
//  Created by Jianguo Wu on 2019/2/18.
//  Copyright © 2019 libms. All rights reserved.
//

import UIKit

class MediaCollectionViewCell: UICollectionViewCell {
    
    static let identifier = "MediaCollectionViewCellIdentifier"
    
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    var item: MediaItem! = nil {
        didSet {
            update()
        }
    }
    
    
    lazy var imageView: UIImageView = {
        let imageView = UIImageView()
        imageView.contentMode = .scaleAspectFill
        imageView.translatesAutoresizingMaskIntoConstraints = false
        return imageView
    }()
    
    func setup() {
        contentView.addSubview(imageView)
        contentView.addConstraint(NSLayoutConstraint(item: imageView, attribute: .top, relatedBy: .equal, toItem: contentView, attribute: .top, multiplier: 1, constant: 0))
        contentView.addConstraint(NSLayoutConstraint(item: imageView, attribute: .left, relatedBy: .equal, toItem: contentView, attribute: .left, multiplier: 1, constant: 0))
        contentView.addConstraint(NSLayoutConstraint(item: imageView, attribute: .right, relatedBy: .equal, toItem: contentView, attribute: .right, multiplier: 1, constant: 0))
        contentView.addConstraint(NSLayoutConstraint(item: imageView, attribute: .bottom, relatedBy: .equal, toItem: contentView, attribute: .bottom, multiplier: 1, constant: 0))
    }
    
    func update() {
        imageView.setImage(url: item.cover)
    }

}
