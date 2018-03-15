/*
* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __HW_DEVICE_DRM_H__
#define __HW_DEVICE_DRM_H__

#include <drm_interface.h>
#include <errno.h>
#include <pthread.h>
#include <xf86drmMode.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "hw_interface.h"
#include "hw_scale_drm.h"

#define IOCTL_LOGE(ioctl, type) \
  DLOGE("ioctl %s, device = %d errno = %d, desc = %s", #ioctl, type, errno, strerror(errno))

namespace sdm {
class HWInfoInterface;

class HWDeviceDRM : public HWInterface {
 public:
  HWDeviceDRM(BufferSyncHandler *buffer_sync_handler, BufferAllocator *buffer_allocator,
                       HWInfoInterface *hw_info_intf);
  virtual ~HWDeviceDRM() {}
  virtual DisplayError Init();
  virtual DisplayError Deinit();
  void GetDRMDisplayToken(sde_drm::DRMDisplayToken *token) const;
  bool IsPrimaryDisplay() const { return hw_panel_info_.is_primary_panel; }

 protected:
  // From HWInterface
  virtual DisplayError GetActiveConfig(uint32_t *active_config);
  virtual DisplayError GetNumDisplayAttributes(uint32_t *count);
  virtual DisplayError GetDisplayAttributes(uint32_t index,
                                            HWDisplayAttributes *display_attributes);
  virtual DisplayError GetHWPanelInfo(HWPanelInfo *panel_info);
  virtual DisplayError SetDisplayAttributes(uint32_t index);
  virtual DisplayError SetDisplayAttributes(const HWDisplayAttributes &display_attributes);
  virtual DisplayError GetConfigIndex(char *mode, uint32_t *index);
  virtual DisplayError PowerOn(int *release_fence);
  virtual DisplayError PowerOff();
  virtual DisplayError Doze(int *release_fence);
  virtual DisplayError DozeSuspend(int *release_fence);
  virtual DisplayError Standby();
  virtual DisplayError Validate(HWLayers *hw_layers);
  virtual DisplayError Commit(HWLayers *hw_layers);
  virtual DisplayError Flush();
  virtual DisplayError GetPPFeaturesVersion(PPFeatureVersion *vers);
  virtual DisplayError SetPPFeatures(PPFeaturesConfig *feature_list);
  // This API is no longer supported, expectation is to call the correct API on HWEvents
  virtual DisplayError SetVSyncState(bool enable);
  virtual void SetIdleTimeoutMs(uint32_t timeout_ms);
  virtual DisplayError SetDisplayMode(const HWDisplayMode hw_display_mode);
  virtual DisplayError SetRefreshRate(uint32_t refresh_rate);
  virtual DisplayError SetPanelBrightness(int level);
  virtual DisplayError GetHWScanInfo(HWScanInfo *scan_info);
  virtual DisplayError GetVideoFormat(uint32_t config_index, uint32_t *video_format);
  virtual DisplayError GetMaxCEAFormat(uint32_t *max_cea_format);
  virtual DisplayError SetCursorPosition(HWLayers *hw_layers, int x, int y);
  virtual DisplayError OnMinHdcpEncryptionLevelChange(uint32_t min_enc_level);
  virtual DisplayError GetPanelBrightness(int *level);
  virtual DisplayError SetAutoRefresh(bool enable) { autorefresh_ = enable; return kErrorNone; }
  virtual DisplayError SetS3DMode(HWS3DMode s3d_mode);
  virtual DisplayError SetScaleLutConfig(HWScaleLutInfo *lut_info);
  virtual DisplayError SetMixerAttributes(const HWMixerAttributes &mixer_attributes);
  virtual DisplayError GetMixerAttributes(HWMixerAttributes *mixer_attributes);
  virtual void InitializeConfigs();
  virtual DisplayError DumpDebugData() { return kErrorNone; }
  virtual void PopulateHWPanelInfo();

  enum {
    kHWEventVSync,
    kHWEventBlank,
  };

  static const int kMaxStringLength = 1024;
  static const int kNumPhysicalDisplays = 2;
  static const int kMaxSysfsCommandLength = 12;
  static constexpr const char *kBrightnessNode =
    "/sys/class/backlight/panel0-backlight/brightness";

  DisplayError SetFormat(const LayerBufferFormat &source, uint32_t *target);
  DisplayError SetStride(HWDeviceType device_type, LayerBufferFormat format, uint32_t width,
                         uint32_t *target);
  DisplayError PopulateDisplayAttributes(uint32_t index);
  void GetHWDisplayPortAndMode();
  void GetHWPanelMaxBrightness();
  bool EnableHotPlugDetection(int enable);
  void UpdateMixerAttributes();
  void SetSolidfillStages();
  void AddSolidfillStage(const HWSolidfillStage &sf, uint32_t plane_alpha);
  void SetBlending(const LayerBlending &source, sde_drm::DRMBlendType *target);
  void SetSrcConfig(const LayerBuffer &input_buffer, const HWRotatorMode &mode, uint32_t *config);
  void SelectCscType(const LayerBuffer &input_buffer, sde_drm::DRMCscType *type);
  void SetRect(const LayerRect &source, sde_drm::DRMRect *target);
  void SetRotation(LayerTransform transform, const HWRotatorMode &mode, uint32_t* rot_bit_mask);
  DisplayError DefaultCommit(HWLayers *hw_layers);
  DisplayError AtomicCommit(HWLayers *hw_layers);
  void SetupAtomic(HWLayers *hw_layers, bool validate);
  void SetSecureConfig(const LayerBuffer &input_buffer, sde_drm::DRMSecureMode *fb_secure_mode,
                       sde_drm::DRMSecurityLevel *security_level);
  bool IsResolutionSwitchEnabled() const { return resolution_switch_enabled_; }
  void SetTopology(sde_drm::DRMTopology drm_topology, HWTopology *hw_topology);
  void SetMultiRectMode(const uint32_t flags, sde_drm::DRMMultiRectMode *target);
  void SetFullROI();

  class Registry {
   public:
    explicit Registry(BufferAllocator *buffer_allocator);
    ~Registry();
    // Called on each Validate and Commit to register layer buffers fds to the slot pointed to by
    // current_index_
    void Register(HWLayers *hw_layers);
    // Clears the slot pointed to by current_index_
    void Unregister();
    // Moves current_index_ to the next position
    void Next();
    // Called on display disconnect to release all gem handles and fb_ids
    void Clear();
    // Maps given fd to FB ID
    void MapBufferToFbId(LayerBuffer* buffer);
    // Finds an fb_id corresponding to an fd in current map
    uint32_t GetFbId(int fd);

   private:
    uint8_t rmfb_delay_ = 1;  // N cycle delay before destroy
    // fd to fb_id map. fd is used as key only for a single draw cycle between
    // prepare and commit. It should not be used for caching in future due to fd recycling
    std::unordered_map<int, uint32_t> *hashmap_ {};
    int current_index_ = 0;
    BufferAllocator *buffer_allocator_ = {};
  };

 protected:
  const char *device_name_ = {};
  bool default_mode_ = false;
  sde_drm::DRMDisplayType disp_type_ = {};
  HWInfoInterface *hw_info_intf_ = {};
  BufferSyncHandler *buffer_sync_handler_ = {};
  int dev_fd_ = -1;
  Registry registry_;
  sde_drm::DRMDisplayToken token_ = {};
  HWResourceInfo hw_resource_ = {};
  HWPanelInfo hw_panel_info_ = {};
  HWScaleDRM *hw_scale_ = {};
  sde_drm::DRMManagerInterface *drm_mgr_intf_ = {};
  sde_drm::DRMAtomicReqInterface *drm_atomic_intf_ = {};
  std::vector<HWDisplayAttributes> display_attributes_ = {};
  uint32_t current_mode_index_ = 0;
  sde_drm::DRMConnectorInfo connector_info_ = {};
  bool first_cycle_ = true;

 private:
  bool synchronous_commit_ = false;
  HWMixerAttributes mixer_attributes_ = {};
  std::string interface_str_ = "DSI";
  std::vector<sde_drm::DRMSolidfillStage> solid_fills_ {};
  bool resolution_switch_enabled_ = false;
  uint32_t vrefresh_ = 0;
  bool autorefresh_ = false;
};

}  // namespace sdm

#endif  // __HW_DEVICE_DRM_H__
