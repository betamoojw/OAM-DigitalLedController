#pragma once


#define paramDelay(time) (uint32_t)( \
            (time & 0xC000) == 0xC000 ? (time & 0x3FFF) * 100 : \
            (time & 0xC000) == 0x0000 ? (time & 0x3FFF) * 1000 : \
            (time & 0xC000) == 0x4000 ? (time & 0x3FFF) * 60000 : \
            (time & 0xC000) == 0x8000 ? ((time & 0x3FFF) > 1000 ? 3600000 : \
                                         (time & 0x3FFF) * 3600000 ) : 0 )
                                             
#define ETS_ModuleId_NONE 0
#define ETS_ModuleId_BASE 1
#define ETS_ModuleId_APP 2
#define MAIN_FirmwareName "Digital LED Controller"
#define MAIN_OpenKnxId 0xAF
#define MAIN_ApplicationNumber 1
#define MAIN_ApplicationVersion 7
#define MAIN_ApplicationEncoding iso-8859-15
#define MAIN_ParameterSize 175
#define MAIN_MaxKoNumber 409
#define MAIN_OrderNumber "OAM-DigitalLEDController"
#define BASE_ModuleVersion 25
#define APP_ModuleVersion 6
// Parameter with single occurrence


#define BASE_StartupDelayBase                     0      // 2 Bits, Bit 7-6
#define     BASE_StartupDelayBaseMask 0xC0
#define     BASE_StartupDelayBaseShift 6
#define BASE_StartupDelayTime                     0      // 14 Bits, Bit 13-0
#define     BASE_StartupDelayTimeMask 0x3FFF
#define     BASE_StartupDelayTimeShift 0
#define BASE_HeartbeatDelayBase                   2      // 2 Bits, Bit 7-6
#define     BASE_HeartbeatDelayBaseMask 0xC0
#define     BASE_HeartbeatDelayBaseShift 6
#define BASE_HeartbeatDelayTime                   2      // 14 Bits, Bit 13-0
#define     BASE_HeartbeatDelayTimeMask 0x3FFF
#define     BASE_HeartbeatDelayTimeShift 0
#define BASE_Timezone                             4      // 5 Bits, Bit 7-3
#define     BASE_TimezoneMask 0xF8
#define     BASE_TimezoneShift 3
#define BASE_CombinedTimeDate                     4      // 1 Bit, Bit 2
#define     BASE_CombinedTimeDateMask 0x04
#define     BASE_CombinedTimeDateShift 2
#define BASE_SummertimeAll                        4      // 2 Bits, Bit 1-0
#define     BASE_SummertimeAllMask 0x03
#define     BASE_SummertimeAllShift 0
#define BASE_SummertimeDE                         4      // 2 Bits, Bit 1-0
#define     BASE_SummertimeDEMask 0x03
#define     BASE_SummertimeDEShift 0
#define BASE_SummertimeWorld                      4      // 2 Bits, Bit 1-0
#define     BASE_SummertimeWorldMask 0x03
#define     BASE_SummertimeWorldShift 0
#define BASE_SummertimeKO                         4      // 2 Bits, Bit 1-0
#define     BASE_SummertimeKOMask 0x03
#define     BASE_SummertimeKOShift 0
#define BASE_TimezoneCustom                       5      // char*, 63 Byte
#define     BASE_TimezoneCustomLength 63
#define BASE_Latitude                            69      // float (4 Byte)
#define BASE_Longitude                           73      // float (4 Byte)
#define BASE_Diagnose                            78      // 1 Bit, Bit 7
#define     BASE_DiagnoseMask 0x80
#define     BASE_DiagnoseShift 7
#define BASE_Watchdog                            78      // 1 Bit, Bit 6
#define     BASE_WatchdogMask 0x40
#define     BASE_WatchdogShift 6
#define BASE_ReadTimeDate                        78      // 1 Bit, Bit 5
#define     BASE_ReadTimeDateMask 0x20
#define     BASE_ReadTimeDateShift 5
#define BASE_HeartbeatExtended                   78      // 1 Bit, Bit 4
#define     BASE_HeartbeatExtendedMask 0x10
#define     BASE_HeartbeatExtendedShift 4
#define BASE_InternalTime                        78      // 1 Bit, Bit 3
#define     BASE_InternalTimeMask 0x08
#define     BASE_InternalTimeShift 3
#define BASE_ManualSave                          78      // 3 Bits, Bit 2-0
#define     BASE_ManualSaveMask 0x07
#define     BASE_ManualSaveShift 0
#define BASE_PeriodicSave                        79      // 8 Bits, Bit 7-0
#define BASE_Info1LedFunc                        80      // 16 Bits, Bit 15-0
#define BASE_Info2LedFunc                        82      // 16 Bits, Bit 15-0
#define BASE_Info3LedFunc                        84      // 16 Bits, Bit 15-0
#define BASE_DefaultLedFunc                      86      // 1 Bit, Bit 7
#define     BASE_DefaultLedFuncMask 0x80
#define     BASE_DefaultLedFuncShift 7
#define BASE_Dummy                               109      // uint8_t
#define BASE_ModuleEnabled_APP                   110      // 1 Bit, Bit 6
#define     BASE_ModuleEnabled_APPMask 0x40
#define     BASE_ModuleEnabled_APPShift 6

// Zeitbasis
#define ParamBASE_StartupDelayBase                    ((knx.paramByte(BASE_StartupDelayBase) & BASE_StartupDelayBaseMask) >> BASE_StartupDelayBaseShift)
// Zeit
#define ParamBASE_StartupDelayTime                    (knx.paramWord(BASE_StartupDelayTime) & BASE_StartupDelayTimeMask)
// Zeit (in Millisekunden)
#define ParamBASE_StartupDelayTimeMS                  (paramDelay(knx.paramWord(BASE_StartupDelayTime)))
// Zeitbasis
#define ParamBASE_HeartbeatDelayBase                  ((knx.paramByte(BASE_HeartbeatDelayBase) & BASE_HeartbeatDelayBaseMask) >> BASE_HeartbeatDelayBaseShift)
// Zeit
#define ParamBASE_HeartbeatDelayTime                  (knx.paramWord(BASE_HeartbeatDelayTime) & BASE_HeartbeatDelayTimeMask)
// Zeit (in Millisekunden)
#define ParamBASE_HeartbeatDelayTimeMS                (paramDelay(knx.paramWord(BASE_HeartbeatDelayTime)))
// Zeitzone
#define ParamBASE_Timezone                            ((knx.paramByte(BASE_Timezone) & BASE_TimezoneMask) >> BASE_TimezoneShift)
// Empfangen über
#define ParamBASE_CombinedTimeDate                    ((bool)(knx.paramByte(BASE_CombinedTimeDate) & BASE_CombinedTimeDateMask))
// Sommerzeit ermitteln durch
#define ParamBASE_SummertimeAll                       (knx.paramByte(BASE_SummertimeAll) & BASE_SummertimeAllMask)
// Sommerzeit ermitteln durch
#define ParamBASE_SummertimeDE                        (knx.paramByte(BASE_SummertimeDE) & BASE_SummertimeDEMask)
// Sommerzeit ermitteln durch
#define ParamBASE_SummertimeWorld                     (knx.paramByte(BASE_SummertimeWorld) & BASE_SummertimeWorldMask)
// Sommerzeit ermitteln durch
#define ParamBASE_SummertimeKO                        (knx.paramByte(BASE_SummertimeKO) & BASE_SummertimeKOMask)
// POSIX TZ-String
#define ParamBASE_TimezoneCustom                      (knx.paramData(BASE_TimezoneCustom))
#define ParamBASE_TimezoneCustomStr                   (knx.paramString(BASE_TimezoneCustom, BASE_TimezoneCustomLength))
// Breitengrad
#define ParamBASE_Latitude                            (knx.paramFloat(BASE_Latitude, Float_Enc_IEEE754Single))
// Längengrad
#define ParamBASE_Longitude                           (knx.paramFloat(BASE_Longitude, Float_Enc_IEEE754Single))
// Diagnoseobjekt anzeigen
#define ParamBASE_Diagnose                            ((bool)(knx.paramByte(BASE_Diagnose) & BASE_DiagnoseMask))
// Watchdog aktivieren
#define ParamBASE_Watchdog                            ((bool)(knx.paramByte(BASE_Watchdog) & BASE_WatchdogMask))
// Bei Neustart vom Bus lesen
#define ParamBASE_ReadTimeDate                        ((bool)(knx.paramByte(BASE_ReadTimeDate) & BASE_ReadTimeDateMask))
// Erweitertes "In Betrieb"
#define ParamBASE_HeartbeatExtended                   ((bool)(knx.paramByte(BASE_HeartbeatExtended) & BASE_HeartbeatExtendedMask))
// InternalTime
#define ParamBASE_InternalTime                        ((bool)(knx.paramByte(BASE_InternalTime) & BASE_InternalTimeMask))
// Manuelles speichern
#define ParamBASE_ManualSave                          (knx.paramByte(BASE_ManualSave) & BASE_ManualSaveMask)
// Zyklisches speichern
#define ParamBASE_PeriodicSave                        (knx.paramByte(BASE_PeriodicSave))
// Info1
#define ParamBASE_Info1LedFunc                        (knx.paramWord(BASE_Info1LedFunc))
// Info2
#define ParamBASE_Info2LedFunc                        (knx.paramWord(BASE_Info2LedFunc))
// Info3
#define ParamBASE_Info3LedFunc                        (knx.paramWord(BASE_Info3LedFunc))
// 
#define ParamBASE_DefaultLedFunc                      ((bool)(knx.paramByte(BASE_DefaultLedFunc) & BASE_DefaultLedFuncMask))
// 
#define ParamBASE_Dummy                               (knx.paramByte(BASE_Dummy))
// APP
#define ParamBASE_ModuleEnabled_APP                   ((bool)(knx.paramByte(BASE_ModuleEnabled_APP) & BASE_ModuleEnabled_APPMask))

#define BASE_KoHeartbeat 1
#define BASE_KoTime 2
#define BASE_KoDate 3
#define BASE_KoDateTime 4
#define BASE_KoIsSummertime 5
#define BASE_KoManualSave 6
#define BASE_KoDiagnose 7

// In Betrieb
#define KoBASE_Heartbeat                           (knx.getGroupObject(BASE_KoHeartbeat))
// Uhrzeit
#define KoBASE_Time                                (knx.getGroupObject(BASE_KoTime))
// Datum
#define KoBASE_Date                                (knx.getGroupObject(BASE_KoDate))
// Uhrzeit/Datum
#define KoBASE_DateTime                            (knx.getGroupObject(BASE_KoDateTime))
// Sommerzeit aktiv
#define KoBASE_IsSummertime                        (knx.getGroupObject(BASE_KoIsSummertime))
// Speichern
#define KoBASE_ManualSave                          (knx.getGroupObject(BASE_KoManualSave))
// Diagnose
#define KoBASE_Diagnose                            (knx.getGroupObject(BASE_KoDiagnose))

#define APP_RequestXInterval                    114      // 8 Bits, Bit 7-0
#define APP_NightlightDuration                  115      // uint8_t
#define APP_NightlightMode                      116      // 2 Bits, Bit 7-6
#define     APP_NightlightModeMask 0xC0
#define     APP_NightlightModeShift 6
#define APP_NightlightTargetBrightness          117      // uint8_t
#define APP_DefaultBrightness                   118      // uint8_t
#define APP_DefaultPalette                      119      // uint8_t
#define APP_DefaultEffect                       120      // uint8_t
#define APP_DefaultEffectSpeed                  121      // uint8_t
#define APP_DefaultEffectIntensity              122      // uint8_t
#define APP_CctKelvinMin                        123      // uint16_t
#define APP_CctKelvinMax                        125      // uint16_t
#define APP_DefaultTransitionMs                 127      // uint16_t
#define APP_SegmentId                           129      // uint8_t
#define APP_EnableSegmentObjects                130      // 1 Bit, Bit 7
#define     APP_EnableSegmentObjectsMask 0x80
#define     APP_EnableSegmentObjectsShift 7
#define APP_EnableSyncObjects                   130      // 1 Bit, Bit 6
#define     APP_EnableSyncObjectsMask 0x40
#define     APP_EnableSyncObjectsShift 6
#define APP_EnableAdvancedObjects               130      // 1 Bit, Bit 5
#define     APP_EnableAdvancedObjectsMask 0x20
#define     APP_EnableAdvancedObjectsShift 5
#define APP_DefaultSyncSendGroups               131      // uint8_t
#define APP_DefaultSyncReceiveGroups            132      // uint8_t
#define APP_SuppressSyncNotifications           133      // 1 Bit, Bit 7
#define     APP_SuppressSyncNotificationsMask 0x80
#define     APP_SuppressSyncNotificationsShift 7
#define APP_EnableEffectOptionObjects           133      // 1 Bit, Bit 6
#define     APP_EnableEffectOptionObjectsMask 0x40
#define     APP_EnableEffectOptionObjectsShift 6
#define APP_EnableSegmentLayoutObjects          133      // 1 Bit, Bit 5
#define     APP_EnableSegmentLayoutObjectsMask 0x20
#define     APP_EnableSegmentLayoutObjectsShift 5
#define APP_EnableSecondaryColorObjects         133      // 1 Bit, Bit 4
#define     APP_EnableSecondaryColorObjectsMask 0x10
#define     APP_EnableSecondaryColorObjectsShift 4
#define APP_EnablePlaylistObjects               133      // 1 Bit, Bit 3
#define     APP_EnablePlaylistObjectsMask 0x08
#define     APP_EnablePlaylistObjectsShift 3
#define APP_EnablePresetManagementObjects       133      // 1 Bit, Bit 2
#define     APP_EnablePresetManagementObjectsMask 0x04
#define     APP_EnablePresetManagementObjectsShift 2
#define APP_EnableRealtimeObjects               133      // 1 Bit, Bit 1
#define     APP_EnableRealtimeObjectsMask 0x02
#define     APP_EnableRealtimeObjectsShift 1
#define APP_EnableHealthObjects                 133      // 1 Bit, Bit 0
#define     APP_EnableHealthObjectsMask 0x01
#define     APP_EnableHealthObjectsShift 0
#define APP_EnableCapabilityObjects             134      // 1 Bit, Bit 7
#define     APP_EnableCapabilityObjectsMask 0x80
#define     APP_EnableCapabilityObjectsShift 7
#define APP_EnableLedmapObjects                 134      // 1 Bit, Bit 6
#define     APP_EnableLedmapObjectsMask 0x40
#define     APP_EnableLedmapObjectsShift 6
#define APP_EnableDestructivePresetActions      134      // 1 Bit, Bit 5
#define     APP_EnableDestructivePresetActionsMask 0x20
#define     APP_EnableDestructivePresetActionsShift 5
#define APP_PlaylistPresetSlot                  135      // uint8_t
#define APP_PlaylistPreset1                     136      // uint8_t
#define APP_PlaylistDuration1                   137      // uint16_t
#define APP_PlaylistTransition1                 139      // uint16_t
#define APP_PlaylistPreset2                     141      // uint8_t
#define APP_PlaylistDuration2                   142      // uint16_t
#define APP_PlaylistTransition2                 144      // uint16_t
#define APP_PlaylistPreset3                     146      // uint8_t
#define APP_PlaylistDuration3                   147      // uint16_t
#define APP_PlaylistTransition3                 149      // uint16_t
#define APP_PlaylistPreset4                     151      // uint8_t
#define APP_PlaylistDuration4                   152      // uint16_t
#define APP_PlaylistTransition4                 154      // uint16_t
#define APP_PlaylistPreset5                     156      // uint8_t
#define APP_PlaylistDuration5                   157      // uint16_t
#define APP_PlaylistTransition5                 159      // uint16_t
#define APP_PlaylistRepeat                      161      // uint8_t
#define APP_PlaylistEnd                         162      // uint8_t
#define APP_PlaylistShuffle                     163      // 1 Bit, Bit 7
#define     APP_PlaylistShuffleMask 0x80
#define     APP_PlaylistShuffleShift 7
#define APP_PresetRandomMin                     164      // uint8_t
#define APP_PresetRandomMax                     165      // uint8_t
#define APP_PresetSaveSegmentBounds             163      // 1 Bit, Bit 6
#define     APP_PresetSaveSegmentBoundsMask 0x40
#define     APP_PresetSaveSegmentBoundsShift 6
#define APP_PresetSaveBrightness                163      // 1 Bit, Bit 5
#define     APP_PresetSaveBrightnessMask 0x20
#define     APP_PresetSaveBrightnessShift 5
#define APP_PresetSaveSelectedSegments          163      // 1 Bit, Bit 4
#define     APP_PresetSaveSelectedSegmentsMask 0x10
#define     APP_PresetSaveSelectedSegmentsShift 4
#define APP_EnableNightlightRuntimeObjects      166      // 1 Bit, Bit 7
#define     APP_EnableNightlightRuntimeObjectsMask 0x80
#define     APP_EnableNightlightRuntimeObjectsShift 7
#define APP_EnableSegmentManagementObjects      166      // 1 Bit, Bit 6
#define     APP_EnableSegmentManagementObjectsMask 0x40
#define     APP_EnableSegmentManagementObjectsShift 6
#define APP_EnablePlaylistRuntimeObjects        166      // 1 Bit, Bit 5
#define     APP_EnablePlaylistRuntimeObjectsMask 0x20
#define     APP_EnablePlaylistRuntimeObjectsShift 5
#define APP_EnableSyncNodeObjects               166      // 1 Bit, Bit 4
#define     APP_EnableSyncNodeObjectsMask 0x10
#define     APP_EnableSyncNodeObjectsShift 4
#define APP_EnablePowerObjects                  166      // 1 Bit, Bit 3
#define     APP_EnablePowerObjectsMask 0x08
#define     APP_EnablePowerObjectsShift 3
#define APP_EnableNetworkObjects                166      // 1 Bit, Bit 2
#define     APP_EnableNetworkObjectsMask 0x04
#define     APP_EnableNetworkObjectsShift 2
#define APP_EnableLedInfoObjects                166      // 1 Bit, Bit 1
#define     APP_EnableLedInfoObjectsMask 0x02
#define     APP_EnableLedInfoObjectsShift 1
#define APP_EnableTimeObjects                   166      // 1 Bit, Bit 0
#define     APP_EnableTimeObjectsMask 0x01
#define     APP_EnableTimeObjectsShift 0
#define APP_EnableDestructiveSegmentActions     167      // 1 Bit, Bit 7
#define     APP_EnableDestructiveSegmentActionsMask 0x80
#define     APP_EnableDestructiveSegmentActionsShift 7
#define APP_PlaylistStopPreset                  168      // uint8_t
#define APP_EnableMultiSegmentObjects           169      // 1 Bit, Bit 7
#define     APP_EnableMultiSegmentObjectsMask 0x80
#define     APP_EnableMultiSegmentObjectsShift 7
#define APP_ManagedSegment1Id                   170      // uint8_t
#define APP_ManagedSegment2Id                   171      // uint8_t
#define APP_ManagedSegment3Id                   172      // uint8_t
#define APP_EnableMatrix2dObjects               169      // 1 Bit, Bit 6
#define     APP_EnableMatrix2dObjectsMask 0x40
#define     APP_EnableMatrix2dObjectsShift 6
#define APP_EnableFilesystemObjects             169      // 1 Bit, Bit 5
#define     APP_EnableFilesystemObjectsMask 0x20
#define     APP_EnableFilesystemObjectsShift 5
#define APP_EnableRealtimeSourceObjects         169      // 1 Bit, Bit 4
#define     APP_EnableRealtimeSourceObjectsMask 0x10
#define     APP_EnableRealtimeSourceObjectsShift 4
#define APP_EnableCommunicationHealthObjects    169      // 1 Bit, Bit 3
#define     APP_EnableCommunicationHealthObjectsMask 0x08
#define     APP_EnableCommunicationHealthObjectsShift 3
#define APP_CommunicationTimeoutSeconds         173      // uint8_t
#define APP_EnablePowerAlarmObjects             169      // 1 Bit, Bit 2
#define     APP_EnablePowerAlarmObjectsMask 0x04
#define     APP_EnablePowerAlarmObjectsShift 2
#define APP_PowerWarningPercent                 174      // uint8_t
#define APP_EnableSegmentCapabilityObjects      169      // 1 Bit, Bit 1
#define     APP_EnableSegmentCapabilityObjectsMask 0x02
#define     APP_EnableSegmentCapabilityObjectsShift 1

// WLED status polling interval
#define ParamAPP_RequestXInterval                    (knx.paramByte(APP_RequestXInterval))
// Nightlight duration (minutes)
#define ParamAPP_NightlightDuration                  (knx.paramByte(APP_NightlightDuration))
// Nightlight mode
#define ParamAPP_NightlightMode                      ((knx.paramByte(APP_NightlightMode) & APP_NightlightModeMask) >> APP_NightlightModeShift)
// Nightlight target brightness
#define ParamAPP_NightlightTargetBrightness          (knx.paramByte(APP_NightlightTargetBrightness))
// Default brightness
#define ParamAPP_DefaultBrightness                   (knx.paramByte(APP_DefaultBrightness))
// Default palette
#define ParamAPP_DefaultPalette                      (knx.paramByte(APP_DefaultPalette))
// Default effect
#define ParamAPP_DefaultEffect                       (knx.paramByte(APP_DefaultEffect))
// Effect speed
#define ParamAPP_DefaultEffectSpeed                  (knx.paramByte(APP_DefaultEffectSpeed))
// Effect intensity
#define ParamAPP_DefaultEffectIntensity              (knx.paramByte(APP_DefaultEffectIntensity))
// Minimum color temperature
#define ParamAPP_CctKelvinMin                        (knx.paramWord(APP_CctKelvinMin))
// Maximum color temperature
#define ParamAPP_CctKelvinMax                        (knx.paramWord(APP_CctKelvinMax))
// Default transition time (ms)
#define ParamAPP_DefaultTransitionMs                 (knx.paramWord(APP_DefaultTransitionMs))
// WLED segment ID
#define ParamAPP_SegmentId                           (knx.paramByte(APP_SegmentId))
// Show segment group objects
#define ParamAPP_EnableSegmentObjects                ((bool)(knx.paramByte(APP_EnableSegmentObjects) & APP_EnableSegmentObjectsMask))
// Show UDP sync group objects
#define ParamAPP_EnableSyncObjects                   ((bool)(knx.paramByte(APP_EnableSyncObjects) & APP_EnableSyncObjectsMask))
// Show advanced group objects
#define ParamAPP_EnableAdvancedObjects               ((bool)(knx.paramByte(APP_EnableAdvancedObjects) & APP_EnableAdvancedObjectsMask))
// Default UDP sync send groups
#define ParamAPP_DefaultSyncSendGroups               (knx.paramByte(APP_DefaultSyncSendGroups))
// Default UDP sync receive groups
#define ParamAPP_DefaultSyncReceiveGroups            (knx.paramByte(APP_DefaultSyncReceiveGroups))
// Do not broadcast KNX changes via UDP sync
#define ParamAPP_SuppressSyncNotifications           ((bool)(knx.paramByte(APP_SuppressSyncNotifications) & APP_SuppressSyncNotificationsMask))
// Show effect parameters
#define ParamAPP_EnableEffectOptionObjects           ((bool)(knx.paramByte(APP_EnableEffectOptionObjects) & APP_EnableEffectOptionObjectsMask))
// Show segment layout controls
#define ParamAPP_EnableSegmentLayoutObjects          ((bool)(knx.paramByte(APP_EnableSegmentLayoutObjects) & APP_EnableSegmentLayoutObjectsMask))
// Show secondary and tertiary color controls
#define ParamAPP_EnableSecondaryColorObjects         ((bool)(knx.paramByte(APP_EnableSecondaryColorObjects) & APP_EnableSecondaryColorObjectsMask))
// Show playlist group objects
#define ParamAPP_EnablePlaylistObjects               ((bool)(knx.paramByte(APP_EnablePlaylistObjects) & APP_EnablePlaylistObjectsMask))
// Show preset management controls
#define ParamAPP_EnablePresetManagementObjects       ((bool)(knx.paramByte(APP_EnablePresetManagementObjects) & APP_EnablePresetManagementObjectsMask))
// Show realtime group objects
#define ParamAPP_EnableRealtimeObjects               ((bool)(knx.paramByte(APP_EnableRealtimeObjects) & APP_EnableRealtimeObjectsMask))
// Show diagnostic group objects
#define ParamAPP_EnableHealthObjects                 ((bool)(knx.paramByte(APP_EnableHealthObjects) & APP_EnableHealthObjectsMask))
// Show WLED capabilities
#define ParamAPP_EnableCapabilityObjects             ((bool)(knx.paramByte(APP_EnableCapabilityObjects) & APP_EnableCapabilityObjectsMask))
// Show LED map and effect timebase controls
#define ParamAPP_EnableLedmapObjects                 ((bool)(knx.paramByte(APP_EnableLedmapObjects) & APP_EnableLedmapObjectsMask))
// Allow saving and deleting presets
#define ParamAPP_EnableDestructivePresetActions      ((bool)(knx.paramByte(APP_EnableDestructivePresetActions) & APP_EnableDestructivePresetActionsMask))
// Preset slot for the playlist
#define ParamAPP_PlaylistPresetSlot                  (knx.paramByte(APP_PlaylistPresetSlot))
// Playlist entry 1: preset
#define ParamAPP_PlaylistPreset1                     (knx.paramByte(APP_PlaylistPreset1))
// Playlist entry 1: duration (100 ms units)
#define ParamAPP_PlaylistDuration1                   (knx.paramWord(APP_PlaylistDuration1))
// Playlist entry 1: transition (100 ms units)
#define ParamAPP_PlaylistTransition1                 (knx.paramWord(APP_PlaylistTransition1))
// Playlist entry 2: preset
#define ParamAPP_PlaylistPreset2                     (knx.paramByte(APP_PlaylistPreset2))
// Playlist entry 2: duration (100 ms units)
#define ParamAPP_PlaylistDuration2                   (knx.paramWord(APP_PlaylistDuration2))
// Playlist entry 2: transition (100 ms units)
#define ParamAPP_PlaylistTransition2                 (knx.paramWord(APP_PlaylistTransition2))
// Playlist entry 3: preset
#define ParamAPP_PlaylistPreset3                     (knx.paramByte(APP_PlaylistPreset3))
// Playlist entry 3: duration (100 ms units)
#define ParamAPP_PlaylistDuration3                   (knx.paramWord(APP_PlaylistDuration3))
// Playlist entry 3: transition (100 ms units)
#define ParamAPP_PlaylistTransition3                 (knx.paramWord(APP_PlaylistTransition3))
// Playlist entry 4: preset (0 = disabled)
#define ParamAPP_PlaylistPreset4                     (knx.paramByte(APP_PlaylistPreset4))
// Playlist entry 4: duration (100 ms units)
#define ParamAPP_PlaylistDuration4                   (knx.paramWord(APP_PlaylistDuration4))
// Playlist entry 4: transition (100 ms units)
#define ParamAPP_PlaylistTransition4                 (knx.paramWord(APP_PlaylistTransition4))
// Playlist entry 5: preset (0 = disabled)
#define ParamAPP_PlaylistPreset5                     (knx.paramByte(APP_PlaylistPreset5))
// Playlist entry 5: duration (100 ms units)
#define ParamAPP_PlaylistDuration5                   (knx.paramWord(APP_PlaylistDuration5))
// Playlist entry 5: transition (100 ms units)
#define ParamAPP_PlaylistTransition5                 (knx.paramWord(APP_PlaylistTransition5))
// Playlist repeat count (0 = repeat indefinitely)
#define ParamAPP_PlaylistRepeat                      (knx.paramByte(APP_PlaylistRepeat))
// Preset to apply when the playlist ends
#define ParamAPP_PlaylistEnd                         (knx.paramByte(APP_PlaylistEnd))
// Shuffle playlist
#define ParamAPP_PlaylistShuffle                     ((bool)(knx.paramByte(APP_PlaylistShuffle) & APP_PlaylistShuffleMask))
// Minimum random preset ID
#define ParamAPP_PresetRandomMin                     (knx.paramByte(APP_PresetRandomMin))
// Maximum random preset ID
#define ParamAPP_PresetRandomMax                     (knx.paramByte(APP_PresetRandomMax))
// Save segment bounds in preset
#define ParamAPP_PresetSaveSegmentBounds             ((bool)(knx.paramByte(APP_PresetSaveSegmentBounds) & APP_PresetSaveSegmentBoundsMask))
// Save brightness in preset
#define ParamAPP_PresetSaveBrightness                ((bool)(knx.paramByte(APP_PresetSaveBrightness) & APP_PresetSaveBrightnessMask))
// Save selected segments only
#define ParamAPP_PresetSaveSelectedSegments          ((bool)(knx.paramByte(APP_PresetSaveSelectedSegments) & APP_PresetSaveSelectedSegmentsMask))
// Show runtime nightlight controls
#define ParamAPP_EnableNightlightRuntimeObjects      ((bool)(knx.paramByte(APP_EnableNightlightRuntimeObjects) & APP_EnableNightlightRuntimeObjectsMask))
// Show segment management controls
#define ParamAPP_EnableSegmentManagementObjects      ((bool)(knx.paramByte(APP_EnableSegmentManagementObjects) & APP_EnableSegmentManagementObjectsMask))
// Show runtime playlist controls
#define ParamAPP_EnablePlaylistRuntimeObjects        ((bool)(knx.paramByte(APP_EnablePlaylistRuntimeObjects) & APP_EnablePlaylistRuntimeObjectsMask))
// Show UDP sync node status
#define ParamAPP_EnableSyncNodeObjects               ((bool)(knx.paramByte(APP_EnableSyncNodeObjects) & APP_EnableSyncNodeObjectsMask))
// Show LED current status
#define ParamAPP_EnablePowerObjects                  ((bool)(knx.paramByte(APP_EnablePowerObjects) & APP_EnablePowerObjectsMask))
// Show network status
#define ParamAPP_EnableNetworkObjects                ((bool)(knx.paramByte(APP_EnableNetworkObjects) & APP_EnableNetworkObjectsMask))
// Show LED and matrix status
#define ParamAPP_EnableLedInfoObjects                ((bool)(knx.paramByte(APP_EnableLedInfoObjects) & APP_EnableLedInfoObjectsMask))
// Show date and time group objects
#define ParamAPP_EnableTimeObjects                   ((bool)(knx.paramByte(APP_EnableTimeObjects) & APP_EnableTimeObjectsMask))
// Allow segment deletion
#define ParamAPP_EnableDestructiveSegmentActions     ((bool)(knx.paramByte(APP_EnableDestructiveSegmentActions) & APP_EnableDestructiveSegmentActionsMask))
// Preset to apply when stopping the playlist
#define ParamAPP_PlaylistStopPreset                  (knx.paramByte(APP_PlaylistStopPreset))
// Show three additional segments
#define ParamAPP_EnableMultiSegmentObjects           ((bool)(knx.paramByte(APP_EnableMultiSegmentObjects) & APP_EnableMultiSegmentObjectsMask))
// Additional segment 1 ID
#define ParamAPP_ManagedSegment1Id                   (knx.paramByte(APP_ManagedSegment1Id))
// Additional segment 2 ID
#define ParamAPP_ManagedSegment2Id                   (knx.paramByte(APP_ManagedSegment2Id))
// Additional segment 3 ID
#define ParamAPP_ManagedSegment3Id                   (knx.paramByte(APP_ManagedSegment3Id))
// Show 2D matrix group objects
#define ParamAPP_EnableMatrix2dObjects               ((bool)(knx.paramByte(APP_EnableMatrix2dObjects) & APP_EnableMatrix2dObjectsMask))
// Show filesystem and LED map status
#define ParamAPP_EnableFilesystemObjects             ((bool)(knx.paramByte(APP_EnableFilesystemObjects) & APP_EnableFilesystemObjectsMask))
// Show realtime source status
#define ParamAPP_EnableRealtimeSourceObjects         ((bool)(knx.paramByte(APP_EnableRealtimeSourceObjects) & APP_EnableRealtimeSourceObjectsMask))
// Show communication diagnostics
#define ParamAPP_EnableCommunicationHealthObjects    ((bool)(knx.paramByte(APP_EnableCommunicationHealthObjects) & APP_EnableCommunicationHealthObjectsMask))
// WLED offline timeout (seconds, 0 = disabled)
#define ParamAPP_CommunicationTimeoutSeconds         (knx.paramByte(APP_CommunicationTimeoutSeconds))
// Show LED current warnings
#define ParamAPP_EnablePowerAlarmObjects             ((bool)(knx.paramByte(APP_EnablePowerAlarmObjects) & APP_EnablePowerAlarmObjectsMask))
// LED current warning threshold (%)
#define ParamAPP_PowerWarningPercent                 (knx.paramByte(APP_PowerWarningPercent))
// Show LED capability status
#define ParamAPP_EnableSegmentCapabilityObjects      ((bool)(knx.paramByte(APP_EnableSegmentCapabilityObjects) & APP_EnableSegmentCapabilityObjectsMask))

#define APP_KoChDimSw 201
#define APP_KoChDimRel 202
#define APP_KoChDimAbs 203
#define APP_KoChDimState 204
#define APP_KoChDimValue 205
#define APP_KoChScene 206
#define APP_KoChSceneState 207
#define APP_KoChRed 208
#define APP_KoChRedValue 209
#define APP_KoChGreen 210
#define APP_KoChGreenValue 211
#define APP_KoChBlue 212
#define APP_KoChBlueValue 213
#define APP_KoChRGB 214
#define APP_KoChRGBValue 215
#define APP_KoChNightlightSw 216
#define APP_KoChNightlightState 217
#define APP_KoChPreset 218
#define APP_KoChPresetValue 219
#define APP_KoChPalette 220
#define APP_KoChPaletteValue 221
#define APP_KoChEffectId 222
#define APP_KoChEffectIdValue 223
#define APP_KoChEffectSpeed 224
#define APP_KoChEffectSpeedValue 225
#define APP_KoChEffectIntensity 226
#define APP_KoChEffectIntensityValue 227
#define APP_KoChWhite 228
#define APP_KoChWhiteValue 229
#define APP_KoChCct 230
#define APP_KoChCctValue 231
#define APP_KoChWarmWhite 232
#define APP_KoChWarmWhiteValue 233
#define APP_KoChColdWhite 234
#define APP_KoChColdWhiteValue 235
#define APP_KoChHue 236
#define APP_KoChHueValue 237
#define APP_KoChSaturation 238
#define APP_KoChSaturationValue 239
#define APP_KoChValue 240
#define APP_KoChValueValue 241
#define APP_KoChRGBW 242
#define APP_KoChRGBWValue 243
#define APP_KoChRedRel 244
#define APP_KoChGreenRel 245
#define APP_KoChBlueRel 246
#define APP_KoChWhiteRel 247
#define APP_KoChWarmWhiteRel 248
#define APP_KoChColdWhiteRel 249
#define APP_KoChHueRel 250
#define APP_KoChSaturationRel 251
#define APP_KoChValueRel 252
#define APP_KoChEffectRel 253
#define APP_KoChSegmentSw 254
#define APP_KoChSegmentState 255
#define APP_KoChSegmentBrightness 256
#define APP_KoChSegmentBrightnessValue 257
#define APP_KoChSegmentReverse 258
#define APP_KoChSegmentReverseState 259
#define APP_KoChSegmentMirror 260
#define APP_KoChSegmentMirrorState 261
#define APP_KoChSegmentFreeze 262
#define APP_KoChSegmentFreezeState 263
#define APP_KoChSegmentSelected 264
#define APP_KoChSegmentSelectedState 265
#define APP_KoChTransition 266
#define APP_KoChTransitionValue 267
#define APP_KoChSyncSend 268
#define APP_KoChSyncSendState 269
#define APP_KoChSyncReceive 270
#define APP_KoChSyncReceiveState 271
#define APP_KoChSyncSendGroups 272
#define APP_KoChSyncSendGroupsValue 273
#define APP_KoChSyncReceiveGroups 274
#define APP_KoChSyncReceiveGroupsValue 275
#define APP_KoChLiveOverride 276
#define APP_KoChLiveOverrideValue 277
#define APP_KoChMainSegment 278
#define APP_KoChMainSegmentValue 279
#define APP_KoChNextPlaylist 280
#define APP_KoChPlaylistValue 281
#define APP_KoChStateRequest 282
#define APP_KoChReboot 283
#define APP_KoChEffectCustom1 284
#define APP_KoChEffectCustom1Value 285
#define APP_KoChEffectCustom2 286
#define APP_KoChEffectCustom2Value 287
#define APP_KoChEffectCustom3 288
#define APP_KoChEffectCustom3Value 289
#define APP_KoChEffectOption1 290
#define APP_KoChEffectOption1State 291
#define APP_KoChEffectOption2 292
#define APP_KoChEffectOption2State 293
#define APP_KoChEffectOption3 294
#define APP_KoChEffectOption3State 295
#define APP_KoChSegmentStart 296
#define APP_KoChSegmentStartValue 297
#define APP_KoChSegmentStop 298
#define APP_KoChSegmentStopValue 299
#define APP_KoChSegmentGrouping 300
#define APP_KoChSegmentGroupingValue 301
#define APP_KoChSegmentSpacing 302
#define APP_KoChSegmentSpacingValue 303
#define APP_KoChSegmentOffset 304
#define APP_KoChSegmentOffsetValue 305
#define APP_KoChSecondaryRGBW 306
#define APP_KoChSecondaryRGBWValue 307
#define APP_KoChTertiaryRGBW 308
#define APP_KoChTertiaryRGBWValue 309
#define APP_KoChPlaylistSave 310
#define APP_KoChPlaylistStart 311
#define APP_KoChPresetActionArm 312
#define APP_KoChPresetActionArmState 313
#define APP_KoChPresetSave 314
#define APP_KoChPresetDelete 315
#define APP_KoChPresetNext 316
#define APP_KoChPresetPrevious 317
#define APP_KoChPresetRandom 318
#define APP_KoChLive 319
#define APP_KoChLiveState 320
#define APP_KoChLiveSegmentValue 321
#define APP_KoChLiveModeValue 322
#define APP_KoChWifiRssiValue 323
#define APP_KoChWifiSignalValue 324
#define APP_KoChUptimeValue 325
#define APP_KoChFreeHeapValue 326
#define APP_KoChErrorValue 327
#define APP_KoChBuildIdValue 328
#define APP_KoChEffectCountValue 329
#define APP_KoChPaletteCountValue 330
#define APP_KoChMaxSegmentsValue 331
#define APP_KoChVersionValue 332
#define APP_KoChLedmap 333
#define APP_KoChLedmapValue 334
#define APP_KoChTimebase 335
#define APP_KoChNightlightDuration 336
#define APP_KoChNightlightDurationValue 337
#define APP_KoChNightlightMode 338
#define APP_KoChNightlightModeValue 339
#define APP_KoChNightlightTarget 340
#define APP_KoChNightlightTargetValue 341
#define APP_KoChNightlightRemainingValue 342
#define APP_KoChSegmentName 343
#define APP_KoChSegmentNameValue 344
#define APP_KoChSegmentPresentState 345
#define APP_KoChSegmentActionArm 346
#define APP_KoChSegmentActionArmState 347
#define APP_KoChSegmentDelete 348
#define APP_KoChSegmentApply 349
#define APP_KoChPlaylistStop 350
#define APP_KoChPlaylistActiveState 351
#define APP_KoChPresetActiveState 352
#define APP_KoChNodeCountValue 353
#define APP_KoChLedCurrentValue 354
#define APP_KoChLedMaxCurrentValue 355
#define APP_KoChIpAddressValue 356
#define APP_KoChWifiBssidValue 357
#define APP_KoChWifiChannelValue 358
#define APP_KoChLedCountValue 359
#define APP_KoChLedFpsValue 360
#define APP_KoChMatrixWidthValue 361
#define APP_KoChMatrixHeightValue 362
#define APP_KoChDeviceTime 363
#define APP_KoChLocalDateValue 364
#define APP_KoChLocalTimeValue 365
#define APP_KoChManagedSegment1On 366
#define APP_KoChManagedSegment1Brightness 367
#define APP_KoChManagedSegment1Cct 368
#define APP_KoChManagedSegment1Rgbw 369
#define APP_KoChManagedSegment1Options 370
#define APP_KoChManagedSegment1Name 371
#define APP_KoChManagedSegment2On 372
#define APP_KoChManagedSegment2Brightness 373
#define APP_KoChManagedSegment2Cct 374
#define APP_KoChManagedSegment2Rgbw 375
#define APP_KoChManagedSegment2Options 376
#define APP_KoChManagedSegment2Name 377
#define APP_KoChManagedSegment3On 378
#define APP_KoChManagedSegment3Brightness 379
#define APP_KoChManagedSegment3Cct 380
#define APP_KoChManagedSegment3Rgbw 381
#define APP_KoChManagedSegment3Options 382
#define APP_KoChManagedSegment3Name 383
#define APP_KoChMatrixStartY 384
#define APP_KoChMatrixStopY 385
#define APP_KoChMatrixReverseY 386
#define APP_KoChMatrixMirrorY 387
#define APP_KoChMatrixTranspose 388
#define APP_KoChSegmentSet 389
#define APP_KoChSoundSimulation 390
#define APP_KoChMap1D2D 391
#define APP_KoChFsUsedValue 392
#define APP_KoChFsTotalValue 393
#define APP_KoChPresetsModifiedValue 394
#define APP_KoChLedMapCountValue 395
#define APP_KoChRealtimeSourceIpValue 396
#define APP_KoChWebsocketClientsValue 397
#define APP_KoChWledOnlineState 398
#define APP_KoChLastResponseAgeValue 399
#define APP_KoChJsonErrorCountValue 400
#define APP_KoChOversizedFrameCountValue 401
#define APP_KoChUartOverflowCountValue 402
#define APP_KoChCurrentPercentValue 403
#define APP_KoChCurrentWarningState 404
#define APP_KoChCurrentExceededState 405
#define APP_KoChHasRgbwState 406
#define APP_KoChHasWhiteState 407
#define APP_KoChHasCctState 408
#define APP_KoChIsMatrixState 409

// Switching
#define KoAPP_ChDimSw                             (knx.getGroupObject(APP_KoChDimSw))
// Relative dimming
#define KoAPP_ChDimRel                            (knx.getGroupObject(APP_KoChDimRel))
// Brightness
#define KoAPP_ChDimAbs                            (knx.getGroupObject(APP_KoChDimAbs))
// On/off status
#define KoAPP_ChDimState                          (knx.getGroupObject(APP_KoChDimState))
// Brightness status
#define KoAPP_ChDimValue                          (knx.getGroupObject(APP_KoChDimValue))
// Scene control
#define KoAPP_ChScene                             (knx.getGroupObject(APP_KoChScene))
// Scene status
#define KoAPP_ChSceneState                        (knx.getGroupObject(APP_KoChSceneState))
// Red
#define KoAPP_ChRed                               (knx.getGroupObject(APP_KoChRed))
// Red status
#define KoAPP_ChRedValue                          (knx.getGroupObject(APP_KoChRedValue))
// Green
#define KoAPP_ChGreen                             (knx.getGroupObject(APP_KoChGreen))
// Green status
#define KoAPP_ChGreenValue                        (knx.getGroupObject(APP_KoChGreenValue))
// Blue
#define KoAPP_ChBlue                              (knx.getGroupObject(APP_KoChBlue))
// Blue status
#define KoAPP_ChBlueValue                         (knx.getGroupObject(APP_KoChBlueValue))
// RGB
#define KoAPP_ChRGB                               (knx.getGroupObject(APP_KoChRGB))
// RGB status
#define KoAPP_ChRGBValue                          (knx.getGroupObject(APP_KoChRGBValue))
// Nightlight
#define KoAPP_ChNightlightSw                      (knx.getGroupObject(APP_KoChNightlightSw))
// Nightlight status
#define KoAPP_ChNightlightState                   (knx.getGroupObject(APP_KoChNightlightState))
// Preset
#define KoAPP_ChPreset                            (knx.getGroupObject(APP_KoChPreset))
// Preset status
#define KoAPP_ChPresetValue                       (knx.getGroupObject(APP_KoChPresetValue))
// Palette
#define KoAPP_ChPalette                           (knx.getGroupObject(APP_KoChPalette))
// Palette status
#define KoAPP_ChPaletteValue                      (knx.getGroupObject(APP_KoChPaletteValue))
// Effect
#define KoAPP_ChEffectId                          (knx.getGroupObject(APP_KoChEffectId))
// Effect status
#define KoAPP_ChEffectIdValue                     (knx.getGroupObject(APP_KoChEffectIdValue))
// Effect speed
#define KoAPP_ChEffectSpeed                       (knx.getGroupObject(APP_KoChEffectSpeed))
// Effect speed status
#define KoAPP_ChEffectSpeedValue                  (knx.getGroupObject(APP_KoChEffectSpeedValue))
// Effect intensity
#define KoAPP_ChEffectIntensity                   (knx.getGroupObject(APP_KoChEffectIntensity))
// Effect intensity status
#define KoAPP_ChEffectIntensityValue              (knx.getGroupObject(APP_KoChEffectIntensityValue))
// White
#define KoAPP_ChWhite                             (knx.getGroupObject(APP_KoChWhite))
// White status
#define KoAPP_ChWhiteValue                        (knx.getGroupObject(APP_KoChWhiteValue))
// Color temperature
#define KoAPP_ChCct                               (knx.getGroupObject(APP_KoChCct))
// Color temperature status
#define KoAPP_ChCctValue                          (knx.getGroupObject(APP_KoChCctValue))
// Warm white
#define KoAPP_ChWarmWhite                         (knx.getGroupObject(APP_KoChWarmWhite))
// Warm white status
#define KoAPP_ChWarmWhiteValue                    (knx.getGroupObject(APP_KoChWarmWhiteValue))
// Cool white
#define KoAPP_ChColdWhite                         (knx.getGroupObject(APP_KoChColdWhite))
// Cool white status
#define KoAPP_ChColdWhiteValue                    (knx.getGroupObject(APP_KoChColdWhiteValue))
// Hue
#define KoAPP_ChHue                               (knx.getGroupObject(APP_KoChHue))
// Hue status
#define KoAPP_ChHueValue                          (knx.getGroupObject(APP_KoChHueValue))
// Saturation
#define KoAPP_ChSaturation                        (knx.getGroupObject(APP_KoChSaturation))
// Saturation status
#define KoAPP_ChSaturationValue                   (knx.getGroupObject(APP_KoChSaturationValue))
// HSV value
#define KoAPP_ChValue                             (knx.getGroupObject(APP_KoChValue))
// HSV value status
#define KoAPP_ChValueValue                        (knx.getGroupObject(APP_KoChValueValue))
// RGBW
#define KoAPP_ChRGBW                              (knx.getGroupObject(APP_KoChRGBW))
// RGBW status
#define KoAPP_ChRGBWValue                         (knx.getGroupObject(APP_KoChRGBWValue))
// Relative adjustment: red
#define KoAPP_ChRedRel                            (knx.getGroupObject(APP_KoChRedRel))
// Relative adjustment: green
#define KoAPP_ChGreenRel                          (knx.getGroupObject(APP_KoChGreenRel))
// Relative adjustment: blue
#define KoAPP_ChBlueRel                           (knx.getGroupObject(APP_KoChBlueRel))
// Relative adjustment: white
#define KoAPP_ChWhiteRel                          (knx.getGroupObject(APP_KoChWhiteRel))
// Relative adjustment: warm white
#define KoAPP_ChWarmWhiteRel                      (knx.getGroupObject(APP_KoChWarmWhiteRel))
// Relative adjustment: cool white
#define KoAPP_ChColdWhiteRel                      (knx.getGroupObject(APP_KoChColdWhiteRel))
// Relative adjustment: hue
#define KoAPP_ChHueRel                            (knx.getGroupObject(APP_KoChHueRel))
// Relative adjustment: saturation
#define KoAPP_ChSaturationRel                     (knx.getGroupObject(APP_KoChSaturationRel))
// Relative adjustment: HSV value
#define KoAPP_ChValueRel                          (knx.getGroupObject(APP_KoChValueRel))
// Relative adjustment: effect index
#define KoAPP_ChEffectRel                         (knx.getGroupObject(APP_KoChEffectRel))
// Segment switching
#define KoAPP_ChSegmentSw                         (knx.getGroupObject(APP_KoChSegmentSw))
// Segment on/off status
#define KoAPP_ChSegmentState                      (knx.getGroupObject(APP_KoChSegmentState))
// Segment brightness
#define KoAPP_ChSegmentBrightness                 (knx.getGroupObject(APP_KoChSegmentBrightness))
// Segment brightness status
#define KoAPP_ChSegmentBrightnessValue            (knx.getGroupObject(APP_KoChSegmentBrightnessValue))
// Reverse segment
#define KoAPP_ChSegmentReverse                    (knx.getGroupObject(APP_KoChSegmentReverse))
// Segment reverse status
#define KoAPP_ChSegmentReverseState               (knx.getGroupObject(APP_KoChSegmentReverseState))
// Mirror segment
#define KoAPP_ChSegmentMirror                     (knx.getGroupObject(APP_KoChSegmentMirror))
// Segment mirror status
#define KoAPP_ChSegmentMirrorState                (knx.getGroupObject(APP_KoChSegmentMirrorState))
// Freeze segment
#define KoAPP_ChSegmentFreeze                     (knx.getGroupObject(APP_KoChSegmentFreeze))
// Segment freeze status
#define KoAPP_ChSegmentFreezeState                (knx.getGroupObject(APP_KoChSegmentFreezeState))
// Select segment
#define KoAPP_ChSegmentSelected                   (knx.getGroupObject(APP_KoChSegmentSelected))
// Segment selection status
#define KoAPP_ChSegmentSelectedState              (knx.getGroupObject(APP_KoChSegmentSelectedState))
// Transition time
#define KoAPP_ChTransition                        (knx.getGroupObject(APP_KoChTransition))
// Transition time status
#define KoAPP_ChTransitionValue                   (knx.getGroupObject(APP_KoChTransitionValue))
// UDP sync send
#define KoAPP_ChSyncSend                          (knx.getGroupObject(APP_KoChSyncSend))
// UDP sync send status
#define KoAPP_ChSyncSendState                     (knx.getGroupObject(APP_KoChSyncSendState))
// UDP sync receive
#define KoAPP_ChSyncReceive                       (knx.getGroupObject(APP_KoChSyncReceive))
// UDP sync receive status
#define KoAPP_ChSyncReceiveState                  (knx.getGroupObject(APP_KoChSyncReceiveState))
// UDP sync send groups
#define KoAPP_ChSyncSendGroups                    (knx.getGroupObject(APP_KoChSyncSendGroups))
// UDP sync send groups status
#define KoAPP_ChSyncSendGroupsValue               (knx.getGroupObject(APP_KoChSyncSendGroupsValue))
// UDP sync receive groups
#define KoAPP_ChSyncReceiveGroups                 (knx.getGroupObject(APP_KoChSyncReceiveGroups))
// UDP sync receive groups status
#define KoAPP_ChSyncReceiveGroupsValue            (knx.getGroupObject(APP_KoChSyncReceiveGroupsValue))
// Live override
#define KoAPP_ChLiveOverride                      (knx.getGroupObject(APP_KoChLiveOverride))
// Live override status
#define KoAPP_ChLiveOverrideValue                 (knx.getGroupObject(APP_KoChLiveOverrideValue))
// Main segment ID
#define KoAPP_ChMainSegment                       (knx.getGroupObject(APP_KoChMainSegment))
// Main segment ID status
#define KoAPP_ChMainSegmentValue                  (knx.getGroupObject(APP_KoChMainSegmentValue))
// Next playlist entry
#define KoAPP_ChNextPlaylist                      (knx.getGroupObject(APP_KoChNextPlaylist))
// Playlist ID status
#define KoAPP_ChPlaylistValue                     (knx.getGroupObject(APP_KoChPlaylistValue))
// Request WLED status
#define KoAPP_ChStateRequest                      (knx.getGroupObject(APP_KoChStateRequest))
// Reboot WLED
#define KoAPP_ChReboot                            (knx.getGroupObject(APP_KoChReboot))
// Effect parameter 1
#define KoAPP_ChEffectCustom1                     (knx.getGroupObject(APP_KoChEffectCustom1))
// Effect parameter 1 status
#define KoAPP_ChEffectCustom1Value                (knx.getGroupObject(APP_KoChEffectCustom1Value))
// Effect parameter 2
#define KoAPP_ChEffectCustom2                     (knx.getGroupObject(APP_KoChEffectCustom2))
// Effect parameter 2 status
#define KoAPP_ChEffectCustom2Value                (knx.getGroupObject(APP_KoChEffectCustom2Value))
// Effect parameter 3
#define KoAPP_ChEffectCustom3                     (knx.getGroupObject(APP_KoChEffectCustom3))
// Effect parameter 3 status
#define KoAPP_ChEffectCustom3Value                (knx.getGroupObject(APP_KoChEffectCustom3Value))
// Effect option 1
#define KoAPP_ChEffectOption1                     (knx.getGroupObject(APP_KoChEffectOption1))
// Effect option 1 status
#define KoAPP_ChEffectOption1State                (knx.getGroupObject(APP_KoChEffectOption1State))
// Effect option 2
#define KoAPP_ChEffectOption2                     (knx.getGroupObject(APP_KoChEffectOption2))
// Effect option 2 status
#define KoAPP_ChEffectOption2State                (knx.getGroupObject(APP_KoChEffectOption2State))
// Effect option 3
#define KoAPP_ChEffectOption3                     (knx.getGroupObject(APP_KoChEffectOption3))
// Effect option 3 status
#define KoAPP_ChEffectOption3State                (knx.getGroupObject(APP_KoChEffectOption3State))
// Segment start
#define KoAPP_ChSegmentStart                      (knx.getGroupObject(APP_KoChSegmentStart))
// Segment start status
#define KoAPP_ChSegmentStartValue                 (knx.getGroupObject(APP_KoChSegmentStartValue))
// Segment stop
#define KoAPP_ChSegmentStop                       (knx.getGroupObject(APP_KoChSegmentStop))
// Segment stop status
#define KoAPP_ChSegmentStopValue                  (knx.getGroupObject(APP_KoChSegmentStopValue))
// Segment grouping
#define KoAPP_ChSegmentGrouping                   (knx.getGroupObject(APP_KoChSegmentGrouping))
// Segment grouping status
#define KoAPP_ChSegmentGroupingValue              (knx.getGroupObject(APP_KoChSegmentGroupingValue))
// Segment spacing
#define KoAPP_ChSegmentSpacing                    (knx.getGroupObject(APP_KoChSegmentSpacing))
// Segment spacing status
#define KoAPP_ChSegmentSpacingValue               (knx.getGroupObject(APP_KoChSegmentSpacingValue))
// Segment offset
#define KoAPP_ChSegmentOffset                     (knx.getGroupObject(APP_KoChSegmentOffset))
// Segment offset status
#define KoAPP_ChSegmentOffsetValue                (knx.getGroupObject(APP_KoChSegmentOffsetValue))
// Secondary RGBW color
#define KoAPP_ChSecondaryRGBW                     (knx.getGroupObject(APP_KoChSecondaryRGBW))
// Secondary RGBW color status
#define KoAPP_ChSecondaryRGBWValue                (knx.getGroupObject(APP_KoChSecondaryRGBWValue))
// Tertiary RGBW color
#define KoAPP_ChTertiaryRGBW                      (knx.getGroupObject(APP_KoChTertiaryRGBW))
// Tertiary RGBW color status
#define KoAPP_ChTertiaryRGBWValue                 (knx.getGroupObject(APP_KoChTertiaryRGBWValue))
// Save playlist
#define KoAPP_ChPlaylistSave                      (knx.getGroupObject(APP_KoChPlaylistSave))
// Start playlist
#define KoAPP_ChPlaylistStart                     (knx.getGroupObject(APP_KoChPlaylistStart))
// Arm preset actions
#define KoAPP_ChPresetActionArm                   (knx.getGroupObject(APP_KoChPresetActionArm))
// Preset actions armed status
#define KoAPP_ChPresetActionArmState              (knx.getGroupObject(APP_KoChPresetActionArmState))
// Save preset
#define KoAPP_ChPresetSave                        (knx.getGroupObject(APP_KoChPresetSave))
// Delete preset
#define KoAPP_ChPresetDelete                      (knx.getGroupObject(APP_KoChPresetDelete))
// Next preset
#define KoAPP_ChPresetNext                        (knx.getGroupObject(APP_KoChPresetNext))
// Previous preset
#define KoAPP_ChPresetPrevious                    (knx.getGroupObject(APP_KoChPresetPrevious))
// Random preset
#define KoAPP_ChPresetRandom                      (knx.getGroupObject(APP_KoChPresetRandom))
// Realtime mode
#define KoAPP_ChLive                              (knx.getGroupObject(APP_KoChLive))
// Realtime mode status
#define KoAPP_ChLiveState                         (knx.getGroupObject(APP_KoChLiveState))
// Realtime segment status
#define KoAPP_ChLiveSegmentValue                  (knx.getGroupObject(APP_KoChLiveSegmentValue))
// Realtime protocol status
#define KoAPP_ChLiveModeValue                     (knx.getGroupObject(APP_KoChLiveModeValue))
// Wi-Fi RSSI
#define KoAPP_ChWifiRssiValue                     (knx.getGroupObject(APP_KoChWifiRssiValue))
// Wi-Fi signal strength
#define KoAPP_ChWifiSignalValue                   (knx.getGroupObject(APP_KoChWifiSignalValue))
// Uptime
#define KoAPP_ChUptimeValue                       (knx.getGroupObject(APP_KoChUptimeValue))
// Free heap memory
#define KoAPP_ChFreeHeapValue                     (knx.getGroupObject(APP_KoChFreeHeapValue))
// WLED error code
#define KoAPP_ChErrorValue                        (knx.getGroupObject(APP_KoChErrorValue))
// WLED build ID
#define KoAPP_ChBuildIdValue                      (knx.getGroupObject(APP_KoChBuildIdValue))
// Effect count
#define KoAPP_ChEffectCountValue                  (knx.getGroupObject(APP_KoChEffectCountValue))
// Palette count
#define KoAPP_ChPaletteCountValue                 (knx.getGroupObject(APP_KoChPaletteCountValue))
// Maximum segment count
#define KoAPP_ChMaxSegmentsValue                  (knx.getGroupObject(APP_KoChMaxSegmentsValue))
// WLED version
#define KoAPP_ChVersionValue                      (knx.getGroupObject(APP_KoChVersionValue))
// LED map
#define KoAPP_ChLedmap                            (knx.getGroupObject(APP_KoChLedmap))
// LED map status
#define KoAPP_ChLedmapValue                       (knx.getGroupObject(APP_KoChLedmapValue))
// Effect timebase
#define KoAPP_ChTimebase                          (knx.getGroupObject(APP_KoChTimebase))
// Nightlight duration
#define KoAPP_ChNightlightDuration                (knx.getGroupObject(APP_KoChNightlightDuration))
// Nightlight duration status
#define KoAPP_ChNightlightDurationValue           (knx.getGroupObject(APP_KoChNightlightDurationValue))
// Nightlight mode
#define KoAPP_ChNightlightMode                    (knx.getGroupObject(APP_KoChNightlightMode))
// Nightlight mode status
#define KoAPP_ChNightlightModeValue               (knx.getGroupObject(APP_KoChNightlightModeValue))
// Nightlight target brightness
#define KoAPP_ChNightlightTarget                  (knx.getGroupObject(APP_KoChNightlightTarget))
// Nightlight target brightness status
#define KoAPP_ChNightlightTargetValue             (knx.getGroupObject(APP_KoChNightlightTargetValue))
// Nightlight time remaining
#define KoAPP_ChNightlightRemainingValue          (knx.getGroupObject(APP_KoChNightlightRemainingValue))
// Segment name
#define KoAPP_ChSegmentName                       (knx.getGroupObject(APP_KoChSegmentName))
// Segment name status
#define KoAPP_ChSegmentNameValue                  (knx.getGroupObject(APP_KoChSegmentNameValue))
// Segment present status
#define KoAPP_ChSegmentPresentState               (knx.getGroupObject(APP_KoChSegmentPresentState))
// Arm segment deletion
#define KoAPP_ChSegmentActionArm                  (knx.getGroupObject(APP_KoChSegmentActionArm))
// Segment deletion armed status
#define KoAPP_ChSegmentActionArmState             (knx.getGroupObject(APP_KoChSegmentActionArmState))
// Delete segment
#define KoAPP_ChSegmentDelete                     (knx.getGroupObject(APP_KoChSegmentDelete))
// Apply segment layout
#define KoAPP_ChSegmentApply                      (knx.getGroupObject(APP_KoChSegmentApply))
// Stop playlist
#define KoAPP_ChPlaylistStop                      (knx.getGroupObject(APP_KoChPlaylistStop))
// Playlist active status
#define KoAPP_ChPlaylistActiveState               (knx.getGroupObject(APP_KoChPlaylistActiveState))
// Preset active status
#define KoAPP_ChPresetActiveState                 (knx.getGroupObject(APP_KoChPresetActiveState))
// UDP sync node count
#define KoAPP_ChNodeCountValue                    (knx.getGroupObject(APP_KoChNodeCountValue))
// Estimated LED current
#define KoAPP_ChLedCurrentValue                   (knx.getGroupObject(APP_KoChLedCurrentValue))
// LED current limit
#define KoAPP_ChLedMaxCurrentValue                (knx.getGroupObject(APP_KoChLedMaxCurrentValue))
// IPv4 address
#define KoAPP_ChIpAddressValue                    (knx.getGroupObject(APP_KoChIpAddressValue))
// Wi-Fi BSSID
#define KoAPP_ChWifiBssidValue                    (knx.getGroupObject(APP_KoChWifiBssidValue))
// Wi-Fi channel
#define KoAPP_ChWifiChannelValue                  (knx.getGroupObject(APP_KoChWifiChannelValue))
// LED count
#define KoAPP_ChLedCountValue                     (knx.getGroupObject(APP_KoChLedCountValue))
// LED frame rate
#define KoAPP_ChLedFpsValue                       (knx.getGroupObject(APP_KoChLedFpsValue))
// Matrix width
#define KoAPP_ChMatrixWidthValue                  (knx.getGroupObject(APP_KoChMatrixWidthValue))
// Matrix height
#define KoAPP_ChMatrixHeightValue                 (knx.getGroupObject(APP_KoChMatrixHeightValue))
// Set WLED time
#define KoAPP_ChDeviceTime                        (knx.getGroupObject(APP_KoChDeviceTime))
// Local date
#define KoAPP_ChLocalDateValue                    (knx.getGroupObject(APP_KoChLocalDateValue))
// Local time
#define KoAPP_ChLocalTimeValue                    (knx.getGroupObject(APP_KoChLocalTimeValue))
// Segment 1 on/off
#define KoAPP_ChManagedSegment1On                 (knx.getGroupObject(APP_KoChManagedSegment1On))
// Segment 1 brightness
#define KoAPP_ChManagedSegment1Brightness         (knx.getGroupObject(APP_KoChManagedSegment1Brightness))
// Segment 1 color temperature
#define KoAPP_ChManagedSegment1Cct                (knx.getGroupObject(APP_KoChManagedSegment1Cct))
// Segment 1 RGBW
#define KoAPP_ChManagedSegment1Rgbw               (knx.getGroupObject(APP_KoChManagedSegment1Rgbw))
// Segment 1 options
#define KoAPP_ChManagedSegment1Options            (knx.getGroupObject(APP_KoChManagedSegment1Options))
// Segment 1 name
#define KoAPP_ChManagedSegment1Name               (knx.getGroupObject(APP_KoChManagedSegment1Name))
// Segment 2 on/off
#define KoAPP_ChManagedSegment2On                 (knx.getGroupObject(APP_KoChManagedSegment2On))
// Segment 2 brightness
#define KoAPP_ChManagedSegment2Brightness         (knx.getGroupObject(APP_KoChManagedSegment2Brightness))
// Segment 2 color temperature
#define KoAPP_ChManagedSegment2Cct                (knx.getGroupObject(APP_KoChManagedSegment2Cct))
// Segment 2 RGBW
#define KoAPP_ChManagedSegment2Rgbw               (knx.getGroupObject(APP_KoChManagedSegment2Rgbw))
// Segment 2 options
#define KoAPP_ChManagedSegment2Options            (knx.getGroupObject(APP_KoChManagedSegment2Options))
// Segment 2 name
#define KoAPP_ChManagedSegment2Name               (knx.getGroupObject(APP_KoChManagedSegment2Name))
// Segment 3 on/off
#define KoAPP_ChManagedSegment3On                 (knx.getGroupObject(APP_KoChManagedSegment3On))
// Segment 3 brightness
#define KoAPP_ChManagedSegment3Brightness         (knx.getGroupObject(APP_KoChManagedSegment3Brightness))
// Segment 3 color temperature
#define KoAPP_ChManagedSegment3Cct                (knx.getGroupObject(APP_KoChManagedSegment3Cct))
// Segment 3 RGBW
#define KoAPP_ChManagedSegment3Rgbw               (knx.getGroupObject(APP_KoChManagedSegment3Rgbw))
// Segment 3 options
#define KoAPP_ChManagedSegment3Options            (knx.getGroupObject(APP_KoChManagedSegment3Options))
// Segment 3 name
#define KoAPP_ChManagedSegment3Name               (knx.getGroupObject(APP_KoChManagedSegment3Name))
// Matrix start Y
#define KoAPP_ChMatrixStartY                      (knx.getGroupObject(APP_KoChMatrixStartY))
// Matrix stop Y
#define KoAPP_ChMatrixStopY                       (knx.getGroupObject(APP_KoChMatrixStopY))
// Reverse matrix Y
#define KoAPP_ChMatrixReverseY                    (knx.getGroupObject(APP_KoChMatrixReverseY))
// Mirror matrix Y
#define KoAPP_ChMatrixMirrorY                     (knx.getGroupObject(APP_KoChMatrixMirrorY))
// Transpose matrix axes
#define KoAPP_ChMatrixTranspose                   (knx.getGroupObject(APP_KoChMatrixTranspose))
// Segment set
#define KoAPP_ChSegmentSet                        (knx.getGroupObject(APP_KoChSegmentSet))
// Sound simulation
#define KoAPP_ChSoundSimulation                   (knx.getGroupObject(APP_KoChSoundSimulation))
// 1D-to-2D mapping
#define KoAPP_ChMap1D2D                           (knx.getGroupObject(APP_KoChMap1D2D))
// Filesystem space used
#define KoAPP_ChFsUsedValue                       (knx.getGroupObject(APP_KoChFsUsedValue))
// Total filesystem space
#define KoAPP_ChFsTotalValue                      (knx.getGroupObject(APP_KoChFsTotalValue))
// Preset file modification time
#define KoAPP_ChPresetsModifiedValue              (knx.getGroupObject(APP_KoChPresetsModifiedValue))
// LED map count
#define KoAPP_ChLedMapCountValue                  (knx.getGroupObject(APP_KoChLedMapCountValue))
// Realtime source IPv4 address
#define KoAPP_ChRealtimeSourceIpValue             (knx.getGroupObject(APP_KoChRealtimeSourceIpValue))
// WebSocket client count
#define KoAPP_ChWebsocketClientsValue             (knx.getGroupObject(APP_KoChWebsocketClientsValue))
// WLED online status
#define KoAPP_ChWledOnlineState                   (knx.getGroupObject(APP_KoChWledOnlineState))
// Time since last WLED response
#define KoAPP_ChLastResponseAgeValue              (knx.getGroupObject(APP_KoChLastResponseAgeValue))
// JSON error count
#define KoAPP_ChJsonErrorCountValue               (knx.getGroupObject(APP_KoChJsonErrorCountValue))
// Oversized frame count
#define KoAPP_ChOversizedFrameCountValue          (knx.getGroupObject(APP_KoChOversizedFrameCountValue))
// UART overflow count
#define KoAPP_ChUartOverflowCountValue            (knx.getGroupObject(APP_KoChUartOverflowCountValue))
// LED current as a percentage of the limit
#define KoAPP_ChCurrentPercentValue               (knx.getGroupObject(APP_KoChCurrentPercentValue))
// LED current warning status
#define KoAPP_ChCurrentWarningState               (knx.getGroupObject(APP_KoChCurrentWarningState))
// LED current limit exceeded status
#define KoAPP_ChCurrentExceededState              (knx.getGroupObject(APP_KoChCurrentExceededState))
// RGBW supported
#define KoAPP_ChHasRgbwState                      (knx.getGroupObject(APP_KoChHasRgbwState))
// White channel supported
#define KoAPP_ChHasWhiteState                     (knx.getGroupObject(APP_KoChHasWhiteState))
// CCT supported
#define KoAPP_ChHasCctState                       (knx.getGroupObject(APP_KoChHasCctState))
// 2D matrix active status
#define KoAPP_ChIsMatrixState                     (knx.getGroupObject(APP_KoChIsMatrixState))


// enumeration types


#ifdef MAIN_FirmwareRevision
#ifndef FIRMWARE_REVISION
#define FIRMWARE_REVISION MAIN_FirmwareRevision
#endif
#endif
#ifdef MAIN_FirmwareName
#ifndef FIRMWARE_NAME
#define FIRMWARE_NAME MAIN_FirmwareName
#endif
#endif
