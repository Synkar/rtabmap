/**
 * Adapted from SuperPoint.h to Intel OpenVINO Runtime
 */

 #ifndef SUPERPOINT_OPENVINO_H
 #define SUPERPOINT_OPENVINO_H
 
 #include <openvino/openvino.hpp>
 #include <opencv2/opencv.hpp>
 #include "rtabmap/core/Features2d.h"
 #include <vector>
 #include <string>
 
 namespace rtabmap
 {
 
 class SPDetectorOpenVINO {
 public:
     SPDetectorOpenVINO(const std::string & modelPath, float threshold = 0.2f, bool nms = true, int minDistance = 4, const std::string & device = "CPU");
     virtual ~SPDetectorOpenVINO();
     
     std::vector<cv::KeyPoint> detect(const cv::Mat &img, const cv::Mat & mask, ov::Tensor & outDescCache);
     void forceComputeDesc(const cv::Mat &img, ov::Tensor & outDescCache);
 
     void setThreshold(float threshold) { threshold_ = threshold; }
     void SetNMS(bool enabled) { nms_ = enabled; }
     void setMinDistance(float minDistance) { minDistance_ = minDistance; }
 
 private:
     ov::Core core_;
     ov::CompiledModel compiledModel_;
     ov::InferRequest inferRequest_;
 
     float threshold_;
     bool nms_;
     int minDistance_;
     std::string device_;
 };
 
 class RTABMAP_CORE_EXPORT SuperPointOpenVINO : public Feature2D
 {
 public:
     SuperPointOpenVINO(const ParametersMap & parameters = ParametersMap());
     virtual ~SuperPointOpenVINO();
 
     virtual void parseParameters(const ParametersMap & parameters);
     virtual Feature2D::Type getType() const { return kFeatureSuperPointOpenVINO; }
 
 private:
     virtual std::vector<cv::KeyPoint> generateKeypointsImpl(const cv::Mat & image, const cv::Rect & roi, const cv::Mat & mask = cv::Mat());
     virtual cv::Mat generateDescriptorsImpl(const cv::Mat & image, std::vector<cv::KeyPoint> & keypoints) const;
 
     cv::Ptr<SPDetectorOpenVINO> superPoint_;
 
     std::string path_;
     float threshold_;
     bool nms_;
     int minDistance_;
     std::string device_;
 
     // Cache dinâmico mutável para garantir consistência em chamadas assíncronas do RTAB-Map
     mutable ov::Tensor cachedDescTensor_;
     mutable cv::Mat lastProcessedImage_; 
 };
 
 }
 
 #endif // SUPERPOINT_OPENVINO_H