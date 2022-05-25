//
//  JoinChannelVideo.swift
//  APIExample
//
//  Created by 张乾泽 on 2020/4/17.
//  Copyright © 2020 Agora Corp. All rights reserved.
//


/// Join Channel Video
/// This module show how to join base video comunication.
/// 1.Init shared AgoraRtcEngineKit: AgoraRtcEngineKit.sharedEngine(...).
/// 2.Enable video feature: agoraKit.enableVideo().
/// 3.Join Channel to publish/subscribe audio/video: agoraKit.joinChannel(...).
///
/// More detail: https://docs.agora.io/en/Video/start_call_ios?platform=iOS


import AgoraRtcKit
import AGEVideoLayout

class JoinChannelVideoViewController: BaseViewController {

    @IBOutlet weak var videoContainer: AGEVideoContainer!

    var localVideo = Bundle.loadVideoView(type: .local, audioOnly: false)
    var remoteVideo = Bundle.loadVideoView(type: .remote, audioOnly: false)
    
    var agoraKit: AgoraRtcEngineKit!
    
    // MARK: - LifeCycle
    override func viewDidLoad() {
        super.viewDidLoad()
        setupUI()
        
        guard let channelId = configs["channelName"] as? String,
              let resolution = GlobalSettings.shared.getSetting(key: "resolution")?.selectedOption().value as? CGSize,
              let fps = GlobalSettings.shared.getSetting(key: "fps")?.selectedOption().value as? AgoraVideoFrameRate,
              let orientation = GlobalSettings.shared.getSetting(key: "orientation")?.selectedOption().value as? AgoraVideoOutputOrientationMode else {return}
        
        agoraKit = AgoraRtcEngineKit.sharedEngine(withAppId: KeyCenter.AppId, delegate: self)
        
        // Set audio route to speaker
        agoraKit.setDefaultAudioRouteToSpeakerphone(true)
        
        // Enable video feature(Audio enable default) and config encoder parameters
        agoraKit.enableVideo()
        agoraKit.setVideoEncoderConfiguration(AgoraVideoEncoderConfiguration(size: resolution,
                                                                             frameRate: fps,
                                                                             bitrate: AgoraVideoBitrateStandard,
                                                                             orientationMode: orientation,
                                                                             mirrorMode: .auto))
        
        
        // Preview local video with front camera
        let videoCanvas = AgoraRtcVideoCanvas()
        videoCanvas.view = localVideo.videoView
        videoCanvas.renderMode = .hidden
        agoraKit.setupLocalVideo(videoCanvas)
            
        // You can choose whether to publish and subscribe audio/video with AgoraRtcChannelMediaOptions
        let options = AgoraRtcChannelMediaOptions()
        options.clientRoleType = .of((Int32)(AgoraClientRole.broadcaster.rawValue))
        options.publishAudioTrack = .of(true)
        options.publishCameraTrack = .of(true)
        options.autoSubscribeAudio = .of(true)
        options.autoSubscribeVideo = .of(true)
        let result = agoraKit.joinChannel(byToken: KeyCenter.Token, channelId: channelId, uid: 0, mediaOptions: options)
        if result != 0 {
            /// Error code description: https://docs.agora.io/en/Interactive%20Broadcast/error_rtc
            showAlert(title: "Error", message: "Join channel failed with errorCode: \(result)")
        }
    }
    
    override func didMove(toParent parent: UIViewController?) {
        if parent == nil {
            agoraKit.leaveChannel(nil)
        }
    }
    
    // MARK: - UI
    func setupUI () {
        localVideo.setPlaceholder(text: "Local Host".localized)
        remoteVideo.setPlaceholder(text: "Remote Host".localized)
        videoContainer.layoutStream(views: [localVideo, remoteVideo])
    }
}

// MARK: - AgoraRtcEngineDelegate
extension JoinChannelVideoViewController: AgoraRtcEngineDelegate {
    func rtcEngine(_ engine: AgoraRtcEngineKit, didOccurError errorCode: AgoraErrorCode) {
        /// Error code description: https://docs.agora.io/en/Interactive%20Broadcast/error_rtc
        LogUtils.log(message: "Error occur: \(errorCode)", level: .error)
        showAlert(title: "Error", message: "Error: \(errorCode.description)")
    }

    func rtcEngine(_ engine: AgoraRtcEngineKit, didJoinChannel channel: String, withUid uid: UInt, elapsed: Int) {
        LogUtils.log(message: "Join \(channel) with uid \(uid) elapsed \(elapsed)ms", level: .info)
    }

    func rtcEngine(_ engine: AgoraRtcEngineKit, didJoinedOfUid uid: UInt, elapsed: Int) {
        LogUtils.log(message: "Remote user \(uid) joined elapsed \(elapsed)ms", level: .info)
        
        // Render remote user video frame at a UIView
        let videoCanvas = AgoraRtcVideoCanvas()
        videoCanvas.view = remoteVideo.videoView
        videoCanvas.uid = uid
        videoCanvas.renderMode = .hidden
        agoraKit.setupRemoteVideo(videoCanvas)
    }
    
    func rtcEngine(_ engine: AgoraRtcEngineKit, didOfflineOfUid uid: UInt, reason: AgoraUserOfflineReason) {
        LogUtils.log(message: "Remote user \(uid) offline with reason \(reason)", level: .info)
        
        // Stop render remote user video frame
        let videoCanvas = AgoraRtcVideoCanvas()
        videoCanvas.view = nil
        videoCanvas.uid = uid
        agoraKit.setupRemoteVideo(videoCanvas)
    }

    // MARK: RTC runing stats
    func rtcEngine(_ engine: AgoraRtcEngineKit, reportRtcStats stats: AgoraChannelStats) {
        localVideo.statsInfo?.updateChannelStats(stats)
    }
    
    func rtcEngine(_ engine: AgoraRtcEngineKit, localVideoStats stats: AgoraRtcLocalVideoStats) {
        localVideo.statsInfo?.updateLocalVideoStats(stats)
    }
    
    func rtcEngine(_ engine: AgoraRtcEngineKit, localAudioStats stats: AgoraRtcLocalAudioStats) {
        localVideo.statsInfo?.updateLocalAudioStats(stats)
    }
    
    func rtcEngine(_ engine: AgoraRtcEngineKit, remoteVideoStats stats: AgoraRtcRemoteVideoStats) {
        remoteVideo.statsInfo?.updateVideoStats(stats)
    }
    
    func rtcEngine(_ engine: AgoraRtcEngineKit, remoteAudioStats stats: AgoraRtcRemoteAudioStats) {
        remoteVideo.statsInfo?.updateAudioStats(stats)
    }
}

class JoinChannelVideoEntryViewController : UIViewController {
    @IBOutlet weak var channelTextField: UITextField!
        
    @IBAction func joinBtnClicked(sender: UIButton) {
        guard let channelName = channelTextField.text,
              channelName.lengthOfBytes(using: .utf8) > 0 else {return}
        channelTextField.resignFirstResponder()
        
        let identifier = "JoinChannelVideo"
        let storyBoard: UIStoryboard = UIStoryboard(name: identifier, bundle: nil)
        guard let newViewController = storyBoard.instantiateViewController(withIdentifier: identifier) as? BaseViewController else {return}
        newViewController.title = channelName
        newViewController.configs = ["channelName": channelName]
        self.navigationController?.pushViewController(newViewController, animated: true)
    }
}

