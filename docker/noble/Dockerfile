FROM introlab3it/rtabmap:noble-deps

SHELL ["/bin/bash", "-c"]

RUN groupadd -g 110 render || true
# ============================================================
# Install system dependencies
# ============================================================
RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    curl \
    gnupg \
    gpg \
    ca-certificates \
    apt-transport-https \
    python3 \
    python3-pip \
    python3-dev \
    python3-venv \
    git \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# ============================================================
# Install OpenVINO + Intel GPU/NPU support for Ubuntu 24.04 Noble
# ============================================================
RUN wget -qO- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
    | gpg --dearmor -o /usr/share/keyrings/intel-openvino.gpg && \
    wget -qO- https://repositories.intel.com/gpu/intel-graphics.key \
    | gpg --dearmor -o /usr/share/keyrings/intel-graphics.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/intel-openvino.gpg] https://apt.repos.intel.com/openvino/2025 ubuntu24 main" \
    > /etc/apt/sources.list.d/intel-openvino-2025.list && \
    echo "deb [arch=amd64 signed-by=/usr/share/keyrings/intel-graphics.gpg] https://repositories.intel.com/gpu/ubuntu noble client" \
    > /etc/apt/sources.list.d/intel-gpu-noble.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        openvino-2025.2.0 \
        intel-opencl-icd \
        intel-level-zero-gpu \
        level-zero \
        clinfo \
        vainfo \
        libva2 \
        libva-drm2 \
    && rm -rf /var/lib/apt/lists/*

# ============================================================
# Install Intel NPU runtime dependencies
# Based on OpenVINO official ubuntu24_dev image
# ============================================================
RUN mkdir /tmp/npu_deps && cd /tmp/npu_deps && \
    curl -L -O https://github.com/oneapi-src/level-zero/releases/download/v1.21.9/level-zero_1.21.9+u24.04_amd64.deb && \
    curl -L -O https://github.com/intel/linux-npu-driver/releases/download/v1.17.0/intel-driver-compiler-npu_1.17.0.20250508-14912879441_ubuntu24.04_amd64.deb && \
    curl -L -O https://github.com/intel/linux-npu-driver/releases/download/v1.17.0/intel-fw-npu_1.17.0.20250508-14912879441_ubuntu24.04_amd64.deb && \
    curl -L -O https://github.com/intel/linux-npu-driver/releases/download/v1.17.0/intel-level-zero-npu_1.17.0.20250508-14912879441_ubuntu24.04_amd64.deb && \
    apt-get update && \
    apt-get install --no-install-recommends -y ./*.deb && \
    rm -rf /var/lib/apt/lists/* && \
    rm -rf /tmp/npu_deps

ENV LIBVA_DRIVER_NAME=iHD
ENV GST_VAAPI_ALL_DRIVERS=1
ENV LIBVA_DRIVERS_PATH=/usr/lib/x86_64-linux-gnu/dri
# ============================================================
# Create RTAB-Map data directory
# ============================================================
RUN mkdir -p /root/Documents/RTAB-Map && \
    chmod 777 /root/Documents/RTAB-Map

# ============================================================
# Copy RTAB-Map source code
# ============================================================
COPY . /root/rtabmap

# ============================================================
# Build RTAB-Map with OpenVINO support
# ============================================================
RUN source /ros_entrypoint.sh && \
    OPENVINO_CONFIG=$(find / -name OpenVINOConfig.cmake 2>/dev/null | head -n 1) && \
    if [ -z "${OPENVINO_CONFIG}" ]; then \
        echo "OpenVINOConfig.cmake not found"; \
        exit 1; \
    fi && \
    OPENVINO_DIR_FOUND=$(dirname "${OPENVINO_CONFIG}") && \
    echo "OPENVINO_CONFIG=${OPENVINO_CONFIG}" && \
    echo "OPENVINO_DIR_FOUND=${OPENVINO_DIR_FOUND}" && \
    cd /root/rtabmap/build && \
    cmake \
        -DWITH_OPENGV=ON \
        -DWITH_TORCH=ON \
        -DWITH_OPENVINO=ON \
        -DOpenVINO_DIR=${OPENVINO_DIR_FOUND} \
        -DBUILD_EXAMPLES=OFF \
        -DWITH_PYTHON=ON \
        -DWITH_GRIDMAP=OFF \
        .. && \
    make -j4 && \
    make install && \
    ldconfig

# ============================================================
# Default command
# ============================================================
CMD ["/bin/bash"]