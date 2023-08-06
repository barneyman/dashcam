FROM mcr.microsoft.com/devcontainers/cpp:debian-11 as devbase

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get -y install --no-install-recommends <your-package-list-here>

# update
RUN apt update && apt upgrade -y

RUN apt install -y libboost-all-dev


VOLUME /vids
################


FROM devbase as gstreamer_debug

# to get pango (text-overlay) built
RUN apt install -y libpango1.0-dev
# ?
RUN apt install -y libavfilter-dev

WORKDIR /gstreamer

ENV DEBIAN_FRONTEND="noninteractive"
RUN apt-get -y install \
      python3.9 python3.9-dev python3-distutils python3-pip \
      libmount-dev bison flex pkg-config autotools-dev libffi-dev \
      librtmp-dev libx264-dev libsoup2.4-1 libsoup2.4-dev libpng-dev \
      build-essential ninja-build git xvfb cmake libboost-all-dev

# nvidia gpu support
# ENV NVIDIA_DRIVER_CAPABILITIES=all
# RUN apt-get -y install libnvidia-gl-390 libnvidia-decode-390 libnvidia-encode-390 libgl1-mesa-dev libgl1-mesa-glx

RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.9 9
RUN update-alternatives --set python3 /usr/bin/python3.9

RUN python3 -m pip install meson

ARG GLIB_VERSION=2.66.0
RUN git clone https://github.com/GNOME/glib.git && \
      cd glib && \
      git checkout $GLIB_VERSION && \
      meson build && \
      ninja -C build install

# ARG GOBJECT_INTROSPECTION_VERSION=1.64.1
# RUN git clone https://github.com/GNOME/gobject-introspection.git && \
#       cd gobject-introspection && \
#       git checkout $GOBJECT_INTROSPECTION_VERSION && \
#       meson build && \
#       ninja -C build install

ARG GST_BUILD_VERSION=1.18.4
RUN git clone https://github.com/gstreamer/gst-build.git && \
    cd gst-build && \
    git checkout $GST_BUILD_VERSION

ARG GST_VERSION=1.18.4
RUN cd ./gst-build && \
    meson build/ && \
    ./gst-worktree.py add gst-build-$GST_VERSION $GST_VERSION

RUN cd ./gst-build/gst-build-$GST_VERSION && \
    meson build 
        #-Dgst-plugins-base:pango=enabled    
        # -Dintrospection=enabled \
        # -Dexamples=disabled \
        # -Dgtk_doc=disabled \
        # -Dbenchmarks=disabled \
        # -Dgstreamer:tests=disabled \
        # -Dgstreamer:benchmarks=disabled \
        # -Dgst-plugins-base:tests=enabled \
        # -Dgst-plugins-good:tests=enabled \
        # -Dgst-plugins-bad:tests=enabled \
        # -Dgst-plugins-ugly:tests=enabled \
        # -Dgst-plugins-base:gl=enabled \
        # -Dgst-plugins-bad:nvdec=disabled \
        # -Dgi=disabled \
        # -Dpython=disabled \
        # -Dpygobject=disabled

RUN cd ./gst-build/gst-build-$GST_VERSION && \
    ninja -C build install

ENV LD_LIBRARY_PATH=/usr/local/lib/x86_64-linux-gnu:/usr/local/lib:${LD_LIBRARY_PATH}
ENV GI_TYPELIB_PATH=/usr/lib/x86_64-linux-gnu/girepository-1.0:/usr/local/lib/x86_64-linux-gnu/girepository-1.0

# rpi enc hardware
RUN apt install -y gstreamer1.0-omx-generic
# intel vaapi hw encoding 
RUN apt install -y gstreamer1.0-vaapi

# sql dev
#RUN apt install -y libmariadb-dev
WORKDIR /mariadb
RUN git clone https://github.com/mariadb-corporation/mariadb-connector-c && \
        cd mariadb-connector-c && \
        mkdir build && \
        cd build && \
        cmake .. && \
        make && \
        make install

RUN echo '/usr/local/lib/mariadb' > /etc/ld.so.conf.d/mariadb.conf && \
        ldconfig
#####################


WORKDIR /




FROM devbase as gstreamer_dev


# gstreamer
RUN apt install -y gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl  

# dev
RUN apt install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev 
# dev rtsp-server
RUN apt install -y libgstrtspserver-1.0-dev

RUN apt install -y libpango1.0-dev
# ?
RUN apt install -y libavfilter-dev

# intel vaapi hw encoding 
RUN apt install -y gstreamer1.0-vaapi

# sql dev
RUN apt install -y libmariadb-dev



#####################


WORKDIR /
