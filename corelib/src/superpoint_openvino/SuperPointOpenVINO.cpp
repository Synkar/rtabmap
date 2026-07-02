/**
 * Adapted from SuperPoint.cc to Intel OpenVINO Runtime
 */

 #include "SuperPointOpenVINO.h"
 #include <rtabmap/utilite/ULogger.h>
 #include <rtabmap/utilite/UDirectory.h>
 #include <rtabmap/utilite/UFile.h>
 #include <rtabmap/utilite/UConversion.h>
 #include <cmath>
 #include <limits>
 
 namespace rtabmap
 {
 
 namespace {
     static void l2Normalize(float * data, int dim) {
         float norm = 0.0f;
         for(int i = 0; i < dim; ++i) norm += data[i] * data[i];
         norm = std::sqrt(norm) + 1e-8f;
         for(int i = 0; i < dim; ++i) data[i] /= norm;
     }
 
     static void applyNmsCpu(std::vector<cv::KeyPoint> & keypoints, int radius) {
         if(radius <= 0 || keypoints.empty()) return;
         std::sort(keypoints.begin(), keypoints.end(), [](const cv::KeyPoint & a, const cv::KeyPoint & b){
             return a.response > b.response;
         });
         std::vector<cv::KeyPoint> kept;
         kept.reserve(keypoints.size());
         float radiusSqr = float(radius * radius);
         for(size_t i = 0; i < keypoints.size(); ++i) {
             bool keep = true;
             for(size_t j = 0; j < kept.size(); ++j) {
                 if(std::pow(keypoints[i].pt.x - kept[j].pt.x, 2) + std::pow(keypoints[i].pt.y - kept[j].pt.y, 2) <= radiusSqr) {
                     keep = false;
                     break;
                 }
             }
             if(keep) kept.push_back(keypoints[i]);
         }
         keypoints.swap(kept);
     }
 }
 
 SPDetectorOpenVINO::SPDetectorOpenVINO(const std::string & modelPath, float threshold, bool nms, int minDistance, const std::string & device) :
     threshold_(threshold), nms_(nms), minDistance_(minDistance), device_(device)
 {
     if(modelPath.empty()) return;
     std::string path = uReplaceChar(modelPath, '~', UDirectory::homeDir());
     if(!UFile::exists(path)) return;
 
     try {
         compiledModel_ = core_.compile_model(path, device_);
         inferRequest_ = compiledModel_.create_infer_request();
     } catch(...) {}
 }
 
 SPDetectorOpenVINO::~SPDetectorOpenVINO() {}
 
 std::vector<cv::KeyPoint> SPDetectorOpenVINO::detect(const cv::Mat &img, const cv::Mat & mask, ov::Tensor & outDescCache)
 {
     UASSERT(img.type() == CV_8UC1);
     if(compiledModel_.outputs().empty()) return std::vector<cv::KeyPoint>();
 
     try {
         ov::Shape inputShape = {1, 1, (size_t)img.rows, (size_t)img.cols};
         ov::Tensor inputTensor(ov::element::f32, inputShape);
         float * inputData = inputTensor.data<float>();
         for(int i = 0; i < img.rows * img.cols; ++i) inputData[i] = (float)img.data[i] / 255.0f;
 
         inferRequest_.set_input_tensor(inputTensor);
         inferRequest_.infer();
 
         ov::Tensor outputScores = inferRequest_.get_output_tensor(0);
         outDescCache = inferRequest_.get_output_tensor(1);
 
         const ov::Shape semiShape = outputScores.get_shape();
         const float * semiData = outputScores.data<const float>();
 
         std::vector<float> heatmap;
         int heatH = 0, heatW = 0;
 
         if(semiShape.size() == 3 || (semiShape.size() == 4 && semiShape[1] == 1)) {
             heatH = (semiShape.size() == 3) ? (int)semiShape[1] : (int)semiShape[2];
             heatW = (semiShape.size() == 3) ? (int)semiShape[2] : (int)semiShape[3];
             heatmap.assign(semiData, semiData + heatH * heatW);
         } 
         else if(semiShape.size() == 4 && semiShape[1] == 65) {
             int Hc = (int)semiShape[2], Wc = (int)semiShape[3];
             heatH = Hc * 8; heatW = Wc * 8;
             heatmap.assign(heatH * heatW, 0.0f);
 
             for(int y=0; y<Hc; ++y) {
                 for(int x=0; x<Wc; ++x) {
                     float maxLogit = -std::numeric_limits<float>::infinity();
                     for(int c=0; c<65; ++c) maxLogit = std::max(maxLogit, semiData[((c*Hc) + y) * Wc + x]);
                     float sumExp = 0.0f, probs[64];
                     for(int c=0; c<65; ++c) {
                         float e = std::exp(semiData[((c*Hc) + y) * Wc + x] - maxLogit);
                         if(c < 64) probs[c] = e;
                         sumExp += e;
                     }
                     for(int c=0; c<64; ++c) {
                         heatmap[((y*8 + (c/8))*heatW) + (x*8 + (c%8))] = probs[c] / sumExp;
                     }
                 }
             }
         }
 
         std::vector<cv::KeyPoint> keypoints;
         for(int y = 0; y < heatH && y < img.rows; ++y) {
             for(int x = 0; x < heatW && x < img.cols; ++x) {
                 float score = heatmap[y * heatW + x];
                 if(score < threshold_) continue;
                 if(!mask.empty() && mask.at<unsigned char>(y, x) == 0) continue;
                 keypoints.push_back(cv::KeyPoint((float)x, (float)y, 8.0f, -1.0f, score));
             }
         }
 
         if(nms_) applyNmsCpu(keypoints, minDistance_);
 
         return keypoints;
     } catch(...) {
         return std::vector<cv::KeyPoint>();
     }
 }
 
 void SPDetectorOpenVINO::forceComputeDesc(const cv::Mat &img, ov::Tensor & outDescCache)
 {
     try {
         ov::Shape inputShape = {1, 1, (size_t)img.rows, (size_t)img.cols};
         ov::Tensor inputTensor(ov::element::f32, inputShape);
         float * inputData = inputTensor.data<float>();
         for(int i = 0; i < img.rows * img.cols; ++i) inputData[i] = (float)img.data[i] / 255.0f;
 
         inferRequest_.set_input_tensor(inputTensor);
         inferRequest_.infer();
 
         outDescCache = inferRequest_.get_output_tensor(1);
     } catch(...) {}
 }
 
 SuperPointOpenVINO::SuperPointOpenVINO(const ParametersMap & parameters)
 {
     parseParameters(parameters);
 }
 
 SuperPointOpenVINO::~SuperPointOpenVINO() {}
 
 void SuperPointOpenVINO::parseParameters(const ParametersMap & parameters)
 {
     Feature2D::parseParameters(parameters);
     std::string previousPath = path_;
     std::string previousDevice = device_;
 
     Parameters::parse(parameters, Parameters::kSuperPointOpenVINOModelPath(), path_);
     if(path_.empty()) Parameters::parse(parameters, Parameters::kSuperPointModelPath(), path_);
     
     Parameters::parse(parameters, Parameters::kSuperPointThreshold(), threshold_);
     Parameters::parse(parameters, Parameters::kSuperPointNMS(), nms_);
     Parameters::parse(parameters, Parameters::kSuperPointNMSRadius(), minDistance_);
     Parameters::parse(parameters, Parameters::kSuperPointOpenVINODevice(), device_);
 
     if(superPoint_.get() == 0 || path_.compare(previousPath) != 0 || device_.compare(previousDevice) != 0)
     {
         superPoint_ = cv::Ptr<SPDetectorOpenVINO>(new SPDetectorOpenVINO(path_, threshold_, nms_, minDistance_, device_));
     }
     else
     {
         superPoint_->setThreshold(threshold_);
         superPoint_->SetNMS(nms_);
         superPoint_->setMinDistance(minDistance_);
     }
 }
 
 std::vector<cv::KeyPoint> SuperPointOpenVINO::generateKeypointsImpl(const cv::Mat & image, const cv::Rect & roi, const cv::Mat & mask)
 {
     UASSERT(!image.empty() && image.channels() == 1 && image.depth() == CV_8U);
     
     // Armazena a referência da imagem atual associada ao cache do tensor
     lastProcessedImage_ = image.clone();
     
     return superPoint_->detect(image, mask, cachedDescTensor_);
 }
 
 cv::Mat SuperPointOpenVINO::generateDescriptorsImpl(const cv::Mat & image, std::vector<cv::KeyPoint> & keypoints) const
 {
     UASSERT(!image.empty() && image.channels() == 1 && image.depth() == CV_8U);
     
     if(keypoints.empty()) return cv::Mat();
 
     // SEGURANÇA PARA RE-EXTRAÇÃO EM LOOP CLOSURE / LANDMARKS:
     // Se o cache estiver vazio ou a imagem atual for diferente da última processada no detect(), força a inferência convolucional.
     if(!cachedDescTensor_ || lastProcessedImage_.empty() || 
        image.rows != lastProcessedImage_.rows || image.cols != lastProcessedImage_.cols ||
        std::memcmp(image.data, lastProcessedImage_.data, (image.rows * image.cols)) != 0)
     {
         superPoint_->forceComputeDesc(image, cachedDescTensor_);
         lastProcessedImage_ = image.clone();
     }
 
     if(!cachedDescTensor_) return cv::Mat();
 
     const ov::Shape descShape = cachedDescTensor_.get_shape();
     int C = (int)descShape[1], Hc = (int)descShape[2], Wc = (int)descShape[3];
     const float * descData = cachedDescTensor_.data<const float>();
 
     cv::Mat descriptors((int)keypoints.size(), C, CV_32FC1);
     float cell = 8.0f;
 
     for(size_t i = 0; i < keypoints.size(); ++i) {
         float gx = (keypoints[i].pt.x - cell / 2.0f + 0.5f) / cell;
         float gy = (keypoints[i].pt.y - cell / 2.0f + 0.5f) / cell;
 
         int x0 = (int)std::floor(gx), y0 = (int)std::floor(gy);
         int x1 = x0 + 1, y1 = y0 + 1;
         float dx = gx - (float)x0, dy = gy - (float)y0;
 
         x0 = std::max(0, std::min(x0, Wc - 1)); x1 = std::max(0, std::min(x1, Wc - 1));
         y0 = std::max(0, std::min(y0, Hc - 1)); y1 = std::max(0, std::min(y1, Hc - 1));
 
         float * outRow = descriptors.ptr<float>((int)i);
         for(int c = 0; c < C; ++c) {
             size_t offset = (size_t)c * Hc;
             float v00 = descData[(offset + y0) * Wc + x0];
             float v01 = descData[(offset + y0) * Wc + x1];
             float v10 = descData[(offset + y1) * Wc + x0];
             float v11 = descData[(offset + y1) * Wc + x1];
             outRow[c] = (v00 * (1.0f - dx) + v01 * dx) * (1.0f - dy) + (v10 * (1.0f - dx) + v11 * dx) * dy;
         }
         l2Normalize(outRow, C);
     }
     return descriptors;
 }
 
 } // namespace rtabmap