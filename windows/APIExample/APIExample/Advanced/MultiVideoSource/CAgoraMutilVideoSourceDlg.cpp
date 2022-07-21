#include "stdafx.h"
#include "APIExample.h"
#include "CAgoraMutilVideoSourceDlg.h"

#define FAKE_ROLE_BROADCASTER 0 
#define FAKE_ROLE_AUDIENCE 1


IMPLEMENT_DYNAMIC(CAgoraMutilVideoSourceDlg, CDialogEx)

CAgoraMutilVideoSourceDlg::CAgoraMutilVideoSourceDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MUTI_SOURCE, pParent)
{

}

CAgoraMutilVideoSourceDlg::~CAgoraMutilVideoSourceDlg()
{
}

void CAgoraMutilVideoSourceDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_STATIC_VIDEO, m_staVideoArea);
  DDX_Control(pDX, IDC_LIST_INFO_BROADCASTING, m_lstInfo);
  DDX_Control(pDX, IDC_STATIC_CHANNELNAME, m_staChannel);
  DDX_Control(pDX, IDC_EDIT_CHANNELNAME, m_edtChannel);
  DDX_Control(pDX, IDC_BUTTON_JOINCHANNEL, m_btnJoinChannel);
  DDX_Control(pDX, IDC_BUTTON_PUBLISH, m_btnPublish);
  DDX_Control(pDX, IDC_STATIC_DETAIL, m_staDetail);
  DDX_Control(pDX, IDC_COMBO_ROLE, m_combo_role);
}


BEGIN_MESSAGE_MAP(CAgoraMutilVideoSourceDlg, CDialogEx)
ON_WM_SHOWWINDOW()
ON_MESSAGE(WM_MSGID(EID_JOINCHANNEL_SUCCESS),
           &CAgoraMutilVideoSourceDlg::OnEIDJoinChannelSuccess)
ON_MESSAGE(WM_MSGID(EID_LEAVE_CHANNEL),
           &CAgoraMutilVideoSourceDlg::OnEIDLeaveChannel)
ON_MESSAGE(WM_MSGID(EID_USER_JOINED),
           &CAgoraMutilVideoSourceDlg::OnEIDUserJoined)
ON_MESSAGE(WM_MSGID(EID_USER_OFFLINE),
           &CAgoraMutilVideoSourceDlg::OnEIDUserOffline)
ON_MESSAGE(WM_MSGID(EID_REMOTE_VIDEO_STATE_CHANGED),
           &CAgoraMutilVideoSourceDlg::OnEIDRemoteVideoStateChanged)
ON_BN_CLICKED(IDC_BUTTON_JOINCHANNEL,
              &CAgoraMutilVideoSourceDlg::OnBnClickedButtonJoinchannel)
ON_BN_CLICKED(IDC_BUTTON_PUBLISH,
              &CAgoraMutilVideoSourceDlg::OnBnClickedButtonPublish)
ON_CBN_SELCHANGE(IDC_COMBO_ROLE,
                 &CAgoraMutilVideoSourceDlg::OnCbnSelchangeComboRole)
END_MESSAGE_MAP()


//Initialize the Ctrl Text.
void CAgoraMutilVideoSourceDlg::InitCtrlText()
{

	m_btnPublish.SetWindowText(MultiVideoSourceCtrlPublish);//MultiVideoSourceCtrlUnPublish
	m_staChannel.SetWindowText(commonCtrlChannel);
	m_btnJoinChannel.SetWindowText(commonCtrlJoinChannel);
}


//Initialize the Agora SDK
bool CAgoraMutilVideoSourceDlg::InitAgora() {
  // create Agora RTC engine
  agora::rtc::IRtcEngine* engine = createAgoraRtcEngine();
  m_rtcEngine = (IRtcEngineEx*)engine;
  if (!m_rtcEngine) {
    m_lstInfo.InsertString(m_lstInfo.GetCount() - 1,
                           _T("createAgoraRtcEngine failed"));
    return false;
  }

  m_camera_connection.localUid = generateUid();

  // set message notify receiver window
  eventHandlerCamera.SetMsgReceiver(m_hWnd);
  eventHandlerCamera.SetConnectionId(m_camera_connection.localUid);
  agora::rtc::RtcEngineContext context;
  std::string strAppID = GET_APP_ID;
  context.appId = strAppID.c_str();
  context.eventHandler = &eventHandlerCamera;
  // set channel profile in the engine to the CHANNEL_PROFILE_LIVE_BROADCASTING.
  context.channelProfile = CHANNEL_PROFILE_LIVE_BROADCASTING;
  // initialize the Agora RTC engine context.
  int ret = m_rtcEngine->initialize(context);
  if (ret != 0) {
    m_initialize = false;
    CString strInfo;
    strInfo.Format(_T("initialize failed: %d"), ret);
    m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
    return false;
  } else
    m_initialize = true;
  m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("initialize success"));
  // enable video in the engine.
  m_rtcEngine->enableVideo();
  m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("enable video"));

  m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("live broadcasting"));
  // set client role in the engine to the CLIENT_ROLE_BROADCASTER.
  m_rtcEngine->setClientRole(agora::rtc::CLIENT_ROLE_BROADCASTER);
  m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("setClientRole broadcaster"));

  agora::util::AutoPtr<agora::base::IAgoraParameter> apm;
  agora::base::IAgoraParameter* p = NULL;
  if (engine &&
      !engine->queryInterface(rtc::AGORA_IID_PARAMETER_ENGINE, (void**)&p))
    apm.reset(p);

  apm->setParameters("{\"che.video.local.camera_index\":1024}");
  apm->setParameters("{\"rtc.video.degradation_preference\":100}");
  apm->setParameters("{\"engine.video.enable_hw_encoder\":\"true\"}");
  apm->setParameters("{\"engine.video.enable_hw_decoder\":\"true\"}");
  apm->setParameters("{\"che.video.render.d3d9_texture\":true}");
  apm->setParameters("{\"che.video.win_encoder_preference\":0}");
  apm->setParameters("{\"che.video.minQP\":20}");
  apm->setParameters("{\"che.video.vpr.enable\":false}");
  apm->setParameters("{\"che.video.enableLowBitRateStream\":0}");
  apm->setParameters("{\"rtc.dual_stream_mode\":false}");
  return true;
}


//UnInitialize the Agora SDK
void CAgoraMutilVideoSourceDlg::UnInitAgora()
{
	if (m_rtcEngine) {
		if (m_joinChannel) {
			//leave channel
			m_joinChannel = !m_rtcEngine->leaveChannel();
			m_rtcEngine->leaveChannelEx(m_camera_connection);
		}
		m_bPublishScreen = false;
		//stop preview in the engine.
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("stopPreview"));
		//disable video in the engine.
		m_rtcEngine->disableVideo();
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("disableVideo"));
		//release engine.
		m_rtcEngine->release(true);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("release rtc engine"));
		m_rtcEngine = NULL;
	}
}

//render local video from SDK local capture.
void CAgoraMutilVideoSourceDlg::RenderLocalVideo()
{
	if (m_rtcEngine) {
		//agora::rtc::VideoCanvas canvas;
		//canvas.renderMode = media::base::RENDER_MODE_FIT;
		//canvas.uid = 0;
		//canvas.view = m_videoWnds[0].GetSafeHwnd();
		//canvas.sourceType = VIDEO_SOURCE_CAMERA_PRIMARY;
		////setup local video in the engine to canvas.
		//m_rtcEngine->setupLocalVideo(canvas);
		
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("setupLocalVideo"));
	}
}


//resume window status
void CAgoraMutilVideoSourceDlg::ResumeStatus()
{
	eventHandlerCamera.SetChannelId(0);
	eventHandlerScreen.SetChannelId(0);
	eventHandlerCamera.SetConnectionId(0);
	eventHandlerCamera.SetMsgReceiver(0);
	eventHandlerScreen.SetMsgReceiver(0);
	eventHandlerScreen.SetConnectionId(0);
	InitCtrlText();
	m_joinChannel = false;
	m_initialize = false;
	m_bPublishScreen = false;
	m_btnJoinChannel.EnableWindow(TRUE);
}


void CAgoraMutilVideoSourceDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);
	if (bShow) {
		//init control text.
		InitCtrlText();
		//update window.
		RenderLocalVideo();
	}
	else {
		//resume window status.
		ResumeStatus();
	}
}


BOOL CAgoraMutilVideoSourceDlg::OnInitDialog() {
  CDialogEx::OnInitDialog();
  RECT rcArea;
  m_staVideoArea.GetClientRect(&rcArea);
  RECT leftArea = rcArea;
  leftArea.right = (rcArea.right - rcArea.left) / 2;
  RECT rightArea = rcArea;
  rightArea.left = (rcArea.right - rcArea.left) / 2;

  for (int i = 0; i < this->VIDOE_COUNT; ++i) {
    m_videoWnds[i].Create(
        NULL, NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CRect(0, 0, 1, 1), this, i);
    // set window background color.
    m_videoWnds[i].SetFaceColor(RGB(0x58, 0x58, 0x58));
  }
  m_videoWnds[0].MoveWindow(&leftArea);
  m_videoWnds[1].MoveWindow(&rightArea);

  m_combo_role.AddString(L"broadcaster");
  m_combo_role.AddString(L"audience");
  m_combo_role.SetCurSel(FAKE_ROLE_BROADCASTER);

  // camera screen
  ResumeStatus();
  return TRUE;
}


BOOL CAgoraMutilVideoSourceDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


void CAgoraMutilVideoSourceDlg::StartDesktopShare()
{
	agora::rtc::Rectangle rc;
	ScreenCaptureParameters scp;
	scp.frameRate = 60;
	scp.bitrate = 12000;
	HWND hWnd = ::GetDesktopWindow();
	RECT destop_rc;
	::GetWindowRect(hWnd, &destop_rc);
	scp.dimensions.width = destop_rc.right - destop_rc.left;
	scp.dimensions.height = destop_rc.bottom - destop_rc.top;
	m_rtcEngine->startScreenCaptureByScreenRect(rc, rc, scp);
}

bool CAgoraMutilVideoSourceDlg::ShouldPublishStream() {
  return m_combo_role.GetCurSel() == FAKE_ROLE_BROADCASTER;
}

void CAgoraMutilVideoSourceDlg::OnBnClickedButtonJoinchannel() {
  if (!m_rtcEngine || !m_initialize) return;
  CString strInfo;
  CString strChannelName;
  m_edtChannel.GetWindowText(strChannelName);

  std::string szChannelId = cs2utf8(strChannelName);
  m_camera_connection.channelId = szChannelId.data();

  if (!m_joinChannel) {
    if (strChannelName.IsEmpty()) {
      AfxMessageBox(_T("Fill channel name first"));
      return;
    }

    m_remote_video.clear();
    m_videoWnds[0].Invalidate();
    m_videoWnds[1].Invalidate();
    m_videoWnds[0].SetUID(DEFAULT_CONNECTION_ID);
    m_videoWnds[1].SetUID(DEFAULT_CONNECTION_ID);

    // camera
    agora::rtc::ChannelMediaOptions optionsCamera;
    optionsCamera.autoSubscribeAudio = true;
    optionsCamera.autoSubscribeVideo = true;
    optionsCamera.publishMicrophoneTrack = ShouldPublishStream();
    optionsCamera.publishCameraTrack = ShouldPublishStream();
    optionsCamera.publishScreenTrack = false;
    optionsCamera.clientRoleType = CLIENT_ROLE_BROADCASTER;

    auto config = agora::rtc::VideoEncoderConfiguration(
        agora::rtc::VideoDimensions(960, 720),
        static_cast<agora::rtc::FRAME_RATE>(30), 1500,
        static_cast<agora::rtc::ORIENTATION_MODE>(0));
    config.codecType = agora::rtc::VIDEO_CODEC_H265;
    m_rtcEngine->setVideoEncoderConfiguration(config);

    // join channel in the engine.
    if (0 == m_rtcEngine->joinChannel(APP_TOKEN, szChannelId.data(), 0,
                                      optionsCamera)) {
      strInfo.Format(_T("join channel %s"), strChannelName);
      m_btnJoinChannel.EnableWindow(FALSE);
    }
    m_strChannel = szChannelId;
  } else {
    m_rtcEngine->leaveChannel();
    m_strChannel = "";
  }
}


void CAgoraMutilVideoSourceDlg::OnBnClickedButtonPublish() {
  CString strChannelName;
  m_edtChannel.GetWindowText(strChannelName);

  std::string szChannelId = cs2utf8(strChannelName);

  m_screen_connection.channelId = szChannelId.data();
  m_screen_connection.localUid = generateUid();

  if (!m_bPublishScreen) {
    StartDesktopShare();
    agora::rtc::ChannelMediaOptions options;
    options.autoSubscribeAudio = false;
    options.autoSubscribeVideo = false;
    options.publishScreenTrack = ShouldPublishStream();
    options.publishMicrophoneTrack = false;
    options.publishCameraTrack = false;
    options.clientRoleType = CLIENT_ROLE_BROADCASTER;

    eventHandlerScreen.SetChannelId(0);
    eventHandlerScreen.SetConnectionId(m_screen_connection.localUid);
    eventHandlerScreen.SetMsgReceiver(GetSafeHwnd());

    m_rtcEngine->joinChannelEx(
        APP_TOKEN, m_screen_connection, options,
        &eventHandlerScreen);  // updateChannelMediaOptionsEx(options,
                               // connection);
    m_btnPublish.SetWindowText(MultiVideoSourceCtrlUnPublish);

  } else {
    m_rtcEngine->leaveChannelEx(m_screen_connection);
    m_btnPublish.SetWindowText(MultiVideoSourceCtrlPublish);
    m_videoWnds[1].Invalidate();
  }
  m_bPublishScreen = !m_bPublishScreen;
}

void CAgoraMutilVideoSourceDlg::OnCbnSelchangeComboRole() {
  if (!m_joinChannel) return;

  bool should_pub = ShouldPublishStream();
  agora::rtc::ChannelMediaOptions camera_options;
  camera_options.publishMicrophoneTrack = should_pub;
  camera_options.publishCameraTrack = should_pub; 

  m_rtcEngine->updateChannelMediaOptionsEx(camera_options, m_camera_connection);

  if (m_bPublishScreen) {
    agora::rtc::ChannelMediaOptions screen_options;
    screen_options.publishScreenTrack = should_pub;
    m_rtcEngine->updateChannelMediaOptionsEx(screen_options,
                                             m_screen_connection);
  }
}

//EID_JOINCHANNEL_SUCCESS message window handler.
LRESULT CAgoraMutilVideoSourceDlg::OnEIDJoinChannelSuccess(WPARAM wParam,
                                                           LPARAM lParam) {
  unsigned int cId = (unsigned int)lParam;
  if (cId == m_screen_connection.localUid) return 0;

  CString strChannelName = utf82cs(eventHandlerCamera.GetChannelName());

  m_joinChannel = true;
  m_btnJoinChannel.EnableWindow(TRUE);
  m_btnJoinChannel.SetWindowText(commonCtrlLeaveChannel);
  CString strInfo;
  strInfo.Format(_T("join %s success,cid=%u, uid=%u"), strChannelName, cId,
                 wParam);
  m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);

  return 0;
}

//EID_LEAVE_CHANNEL message window handler.
LRESULT CAgoraMutilVideoSourceDlg::OnEIDLeaveChannel(WPARAM wParam,
                                                     LPARAM lParam) {
  unsigned int cId = (unsigned int)lParam;
  if (cId == m_screen_connection.localUid) return 0;

  m_joinChannel = false;
  CString strChannelName = utf82cs(eventHandlerCamera.GetChannelName());
  m_btnJoinChannel.SetWindowText(commonCtrlJoinChannel);
  CString strInfo;
  strInfo.Format(_T("leave channel:%s "), strChannelName);
  m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);

  return 0;
}

//EID_USER_JOINED message window handler.
LRESULT CAgoraMutilVideoSourceDlg::OnEIDUserJoined(WPARAM wParam,
                                                   LPARAM lParam) {
  unsigned int cId = (unsigned int)lParam;
  if (cId == m_screen_connection.localUid) return 0;

  unsigned int uid = (unsigned int)wParam;

  if (uid == m_screen_connection.localUid) {  // screen
    m_rtcEngine->muteRemoteAudioStream(uid, true);
    m_rtcEngine->muteRemoteVideoStream(uid, true);
    CString strInfo;
    strInfo.Format(_T("local screen %u joined "), uid);
    m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);

    m_lstInfo.InsertString(m_lstInfo.GetCount(), L"camera mute screen");
  } else {
    for (auto& wnd : m_videoWnds) {
      auto target = m_remote_video.find(wnd.GetSafeHwnd());
      if (target != m_remote_video.end())
        continue;  // already exist, find next free wnd to render video

      VideoCanvas canvas;
      canvas.uid = uid;
      canvas.renderMode = media::base::RENDER_MODE_FIT;
      canvas.sourceType = VIDEO_SOURCE_REMOTE;
      canvas.view = wnd.GetSafeHwnd();
      m_rtcEngine->setupRemoteVideo(canvas);

			wnd.SetUID(uid);
			m_remote_video[wnd.GetSafeHwnd()] = uid;
      break;
    }
  }

  return 0;
}


//EID_USER_OFFLINE message window handler.
LRESULT CAgoraMutilVideoSourceDlg::OnEIDUserOffline(WPARAM wParam,
                                                    LPARAM lParam) {
  unsigned int cId = (unsigned int)lParam;
  if (cId == m_screen_connection.localUid) return 0;

  unsigned int target_uid = (unsigned int)wParam;
  auto target_itr = m_remote_video.end();
  for (auto itr = m_remote_video.begin(); itr != m_remote_video.end(); itr++) {
    if (itr->second == target_uid) {
      target_itr = itr;
      break;
    }
  }

  if (target_itr == m_remote_video.end()) return 0;
  VideoCanvas canvas;
  canvas.uid = target_uid;
  m_rtcEngine->setupRemoteVideo(canvas);

	m_remote_video.erase(target_itr);

  return 0;
}

//EID_REMOTE_VIDEO_STATE_CHANGED message window handler.
LRESULT CAgoraMutilVideoSourceDlg::OnEIDRemoteVideoStateChanged(WPARAM wParam, LPARAM lParam)
{
	return 0;
}


/*
note:
	Join the channel callback.This callback method indicates that the client
	successfully joined the specified channel.Channel ids are assigned based
	on the channel name specified in the joinChannel. If IRtcEngine::joinChannel
	is called without a user ID specified. The server will automatically assign one
parameters:
	channel:channel name.
	uid: user ID。If the UID is specified in the joinChannel, that ID is returned here;
	Otherwise, use the ID automatically assigned by the Agora server.
	elapsed: The Time from the joinChannel until this event occurred (ms).
*/
void CAgoraMultiVideoSourceEventHandler::onJoinChannelSuccess(const char* channel, agora::rtc::uid_t uid, int elapsed)
{
	m_strChannel = channel;
	if (m_hMsgHanlder) {
		::PostMessage(m_hMsgHanlder, WM_MSGID(EID_JOINCHANNEL_SUCCESS), (WPARAM)uid, (LPARAM)m_connid);
	}
}

/*
note:
	In the live broadcast scene, each anchor can receive the callback
	of the new anchor joining the channel, and can obtain the uID of the anchor.
	Viewers also receive a callback when a new anchor joins the channel and
	get the anchor's UID.When the Web side joins the live channel, the SDK will
	default to the Web side as long as there is a push stream on the
	Web side and trigger the callback.
parameters:
	uid: remote user/anchor ID for newly added channel.
	elapsed: The joinChannel is called from the local user to the delay triggered
	by the callback(ms).
*/
void CAgoraMultiVideoSourceEventHandler::onUserJoined(agora::rtc::uid_t uid, int elapsed)
{
	if (m_hMsgHanlder) {
		::PostMessage(m_hMsgHanlder, WM_MSGID(EID_USER_JOINED), (WPARAM)uid, (LPARAM)m_connid);
	}
}
/*
note:
	Remote user (communication scenario)/anchor (live scenario) is called back from
	the current channel.A remote user/anchor has left the channel (or dropped the line).
	There are two reasons for users to leave the channel, namely normal departure and
	time-out:When leaving normally, the remote user/anchor will send a message like
	"goodbye". After receiving this message, determine if the user left the channel.
	The basis of timeout dropout is that within a certain period of time
	(live broadcast scene has a slight delay), if the user does not receive any
	packet from the other side, it will be judged as the other side dropout.
	False positives are possible when the network is poor. We recommend using the
	Agora Real-time messaging SDK for reliable drop detection.
parameters:
	uid: The user ID of an offline user or anchor.
	reason:Offline reason: USER_OFFLINE_REASON_TYPE.
*/
void CAgoraMultiVideoSourceEventHandler::onUserOffline(agora::rtc::uid_t uid, agora::rtc::USER_OFFLINE_REASON_TYPE reason)
{
	if (m_hMsgHanlder) {
		::PostMessage(m_hMsgHanlder, WM_MSGID(EID_USER_OFFLINE), (WPARAM)uid, (LPARAM)m_connid);
	}
}
/*
note:
	When the App calls the leaveChannel method, the SDK indicates that the App
	has successfully left the channel. In this callback method, the App can get
	the total call time, the data traffic sent and received by THE SDK and other
	information. The App obtains the call duration and data statistics received
	or sent by the SDK through this callback.
parameters:
	stats: Call statistics.
*/
void CAgoraMultiVideoSourceEventHandler::onLeaveChannel(const agora::rtc::RtcStats& stats)
{
	if (m_hMsgHanlder) {
		::PostMessage(m_hMsgHanlder, WM_MSGID(EID_LEAVE_CHANNEL), (WPARAM)m_channelId, m_connid);
	}
}

void CAgoraMultiVideoSourceEventHandler::onRemoteVideoStateChanged(agora::rtc::uid_t uid, agora::rtc::REMOTE_VIDEO_STATE state, agora::rtc::REMOTE_VIDEO_STATE_REASON reason, int elapsed)
{
	if (m_hMsgHanlder) {
		::PostMessage(m_hMsgHanlder, WM_MSGID(EID_REMOTE_VIDEO_STATE_CHANGED), (WPARAM)uid, (LPARAM)m_channelId);
	}
}
