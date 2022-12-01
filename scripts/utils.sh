#!/usr/bin/env bash

ORANGE='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

downloadAndExtract (){
  local path file # reset first
  local "${@}"

  if [ $SKIP -eq 1 ]
  then
      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Skipping Download ${path} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      return
  fi

  dest_val=""
  if [ ! -z "$SRC_CACHE_DIR" ]; then
    dest_val="$SRC_CACHE_DIR/${file}"
  else
    dest_val=${file}
  fi

  if [ ! -f "$dest_val" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** Downloading : ${path} ***\n${NC}"
    printf "${ORANGE}*** Destination : $dest_val ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    wget ${path} -O $dest_val
  else
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** Source already downloaded : ${path} ***\n${NC}"
    printf "${ORANGE}*** Destination : $dest_val ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** Extracting : ${file} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  if [[ $dest_val == *.tar.gz ]]; then
    tar xfz $dest_val
  elif [[ $dest_val == *.tar.xz ]]; then
    tar xf $dest_val
  elif [[ $dest_val == *.tar.bz2 ]]; then
    tar xjf $dest_val
  else
    echo "ERROR FILE NOT FOUND ${path} // ${file}"
    exit 1
  fi

  if [ -z "$SRC_CACHE_DIR" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** Clean up : $dest_val ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    rm $dest_val
  fi
}

pullOrClone (){
  local path tag depth recurse # reset first
  local "${@}"

  if [ $SKIP -eq 1 ]
  then
      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Skipping Pull/Clone ${tag}@${path} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      return
  fi

  recursestr=""
  if [ ! -z "${recurse}" ] 
  then
    recursestr="--recurse-submodules"
  fi
  depthstr=""
  if [ ! -z "${depth}" ] 
  then
    depthstr="--depth ${depth}"
  fi 

  tgstr=""
  tgstr2=""
  if [ ! -z "${tag}" ] 
  then
    tgstr="origin tags/${tag}"
    tgstr2="-b ${tag}"
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** Cloning ${tag}@${path} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  IFS='/' read -ra ADDR <<< "$path"
  namedotgit=${ADDR[-1]}
  IFS='.' read -ra ADDR <<< "$namedotgit"
  name=${ADDR[0]}

  dest_val=""
  if [ ! -z "$SRC_CACHE_DIR" ]; then
    dest_val="$SRC_CACHE_DIR/$name"
  else
    dest_val=$name
  fi
  if [ ! -z "${tag}" ]; then
    dest_val+="-${tag}"
  fi

  if [ -z "$SRC_CACHE_DIR" ]; then 
    #TODO Check if it's the right tag
    git -C $dest_val pull $tgstr 2> /dev/null || git clone -j$(nproc) $recursestr $depthstr $tgstr2 ${path} $dest_val
  elif [ -d "$dest_val" ] && [ ! -z "${tag}" ]; then #Folder exist, switch to tag
    currenttag=$(git -C $dest_val tag --points-at ${tag})
    echo "TODO Check current tag \"${tag}\" == \"$currenttag\""
    git -C $dest_val pull $tgstr 2> /dev/null || git clone -j$(nproc) $recursestr $depthstr $tgstr2 ${path} $dest_val
  elif [ -d "$dest_val" ] && [ -z "${tag}" ]; then #Folder exist, switch to main
    currenttag=$(git -C $dest_val tag --points-at ${tag})
    echo "TODO Handle no tag \"${tag}\" == \"$currenttag\""
    git -C $dest_val pull $tgstr 2> /dev/null || git clone -j$(nproc) $recursestr $depthstr $tgstr2 ${path} $dest_val
  elif [ ! -d "$dest_val" ]; then #fresh start
    git -C $dest_val pull $tgstr 2> /dev/null || git clone -j$(nproc) $recursestr $depthstr $tgstr2 ${path} $dest_val
  else
    if [ -d "$dest_val" ]; then
      echo "yey destval"
    fi
    if [ -z "${tag}" ]; then
      echo "yey tag"
    fi
    echo "1 $dest_val : $(test -f \"$dest_val\")"
    echo "2 ${tag} : $(test -z \"${tag}\")"
  fi

  if [ ! -z "$SRC_CACHE_DIR" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** Copy repo from cache ***\n${NC}"
    printf "${ORANGE}*** $dest_val ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    rm -rf $name
    cp -r $dest_val ./$name
  fi
}

function displaytime {
    local T=$1
    local D=$((T/60/60/24))
    local H=$((T/60/60%24))
    local M=$((T/60%60))
    local S=$((T%60))
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** "
    (( $D > 0 )) && printf '%d days ' $D
    (( $H > 0 )) && printf '%d hours ' $H
    (( $M > 0 )) && printf '%d minutes ' $M
    (( $D > 0 || $H > 0 || $M > 0 )) && printf 'and '
    printf "%d seconds\n${NC}" $S
    printf "${ORANGE}*****************************\n${NC}"
}


checkPkg (){
    local name prefix atleast exact # reset first
    local "${@}"
    
    ver_val=""
    if [ ! -z "${atleast}" ]; then
      ver_val="--atleast-version=${atleast}"
    elif [ ! -z "${exact}" ]; then
      ver_val="--exact-version=${exact}"
    fi

    pkg-config --exists --print-errors $ver_val ${name}
    ret=$?
    if [ $ret -eq 0 ]; then
      echo "Installed"
    fi
}

checkProg () {
    local name args prefix # reset first
    local "${@}"

    if !  command -v ${name} &> /dev/null
    then
        return #Prog not found
    else
        command ${name} ${args} &> /dev/null
        status=$?
        if [[ $status -eq 0 ]]; then
            echo "Working"
        else
            return #Prog failed
        fi
    fi
}

checkWithUser () {
    if [ $CHECK -ne 1 ] 
    then
        return
    fi

    read -p "Do you want to proceed? (yes/no) " yn

    case $yn in 
        y);;
        ye);;
	yes );;
	no ) echo exiting...;
		exit;;
	* ) echo invalid response;
		exit 1;;
    esac
}

buildMake() {
    local srcdir prefix autoreconf preconfigure configure configcustom cmakedir autogenargs cmakeargs makeargs datarootdir datadir # reset first
    local "${@}"

    build_start=$SECONDS
    if [ $SKIP -eq 1 ]; then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** Skipping Make ${srcdir} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
        return
    fi

    datarootdir_val=""
    if [ ! -z "${datarootdir}" ]; then
        datarootdir_val="--datarootdir=${datarootdir}"
    fi

    datadir_val=""
    if [ ! -z "${datadir}" ]; then
        datadir_val="--datadir=${datadir}"
    fi

    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}* Building Github Project ***\n${NC}"
    printf "${ORANGE}* Src dir : ${srcdir} ***\n${NC}"
    printf "${ORANGE}* Prefix : ${prefix} ***\n${NC}"
    printf "${ORANGE}* datadir : ${datadir_val} ***\n${NC}"
    printf "${ORANGE}* datarootdir : ${datarootdir_val} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"

    cd ${srcdir}

    
    if [ ! -z "${preconfigure}" ]; then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}* Preconfigure              *\n${NC}"
        printf "${ORANGE}* ${preconfigure} *\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
            bash -c "${preconfigure}"
        status=$?
        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** Preconfigure failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
            checkWithUser
            return
        fi
    fi

    ## Bootstrap check
    if [ -f "./bootstrap" ]; then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** bootstrap ${srcdir} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
            ./bootstrap
        status=$?
        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** ./bootstrap failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
        fi
    elif [ -f "./bootstrap.sh" ]; then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** bootstrap.sh ${srcdir} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
            ./bootstrap.sh
        status=$?
        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** ./bootstrap.sh failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
            checkWithUser
            return
        fi
    fi

    ## Autoreconf flag
    if [ ! -z "${autoreconf}" ] 
    then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** autoreconf ${srcdir} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
            autoreconf -fiv
        status=$?
        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** Autoreconf failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
            checkWithUser
            return
        fi
    fi

    ## Autogen check ... fall back to configure
    if [ -f "./autogen.sh" ]; then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** autogen ${srcdir} ***\n${NC}"
        printf "${ORANGE}*** PKG_CONFIG_PATH=$PKG_CONFIG_PATH ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
        PYTHONPATH=$PYTHONPATH \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
        LIBRARY_PATH=$LIBRARY_PATH \
            ./autogen.sh ${autogenargs} \
                --prefix=${prefix} \
                --libdir=${prefix}/lib \
                --includedir=${prefix}/include \
                --bindir=${prefix}/bin \
                --libexecdir=${prefix}/libexec \
                ${datarootdir_val} \
                ${datadir_val} \
                ${configure}
        status=$?

        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** Autogen failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
            checkWithUser
            return
        fi
    elif [ ! -z "${configcustom}" ]; then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** custom config ${srcdir} ***\n${NC}"
        printf "${ORANGE}*** ${configcustom} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
        PYTHONPATH=$PYTHONPATH \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
        LIBRARY_PATH=$LIBRARY_PATH \
            bash -c "${configcustom}"
        status=$?
        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** Custom Config failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
            return
        fi
    elif [ -f "./configure.sh" ]; then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** configure ${srcdir} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
        PYTHONPATH=$PYTHONPATH \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
        LIBRARY_PATH=$LIBRARY_PATH \
            ./configure.sh \
                --prefix=${prefix} \
                --libdir=${prefix}/lib \
                --includedir=${prefix}/include \
                --bindir=${prefix}/bin \
                --libexecdir=${prefix}/libexec \
                ${datarootdir_val} \
                ${datadir_val} \
                ${configure}

        status=$?
        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** ./configure.sh failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
            return
        fi
    elif [ -f "./configure" ]; then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** configure ${srcdir} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
        PYTHONPATH=$PYTHONPATH \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
        LIBRARY_PATH=$LIBRARY_PATH \
            ./configure \
                --prefix=${prefix} \
                --libdir=${prefix}/lib \
                --includedir=${prefix}/include \
                --bindir=${prefix}/bin \
                --libexecdir=${prefix}/libexec \
                ${datarootdir_val} \
                ${datadir_val} \
                ${configure}
        status=$?

        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** ./configure failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
        fi
    else
      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** no configuration available ${srcdir} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
    fi
    
    if [ ! -z "${cmakedir}" ] 
    then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** cmake ${srcdir} ***\n${NC}"
        printf "${ORANGE}*** Args ${cmakeargs} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
        PYTHONPATH=$PYTHONPATH \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
        LIBRARY_PATH=$LIBRARY_PATH \
        cmake -G "Unix Makefiles" \
            ${cmakeargs} \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="${prefix}" \
            -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${prefix}/lib \
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${prefix}/lib \
            -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${prefix}/bin \
            -DENABLE_TESTS=OFF \
            -DENABLE_SHARED=on \
            -DENABLE_NASM=on \
            -DPYTHON_EXECUTABLE="$(which python3)" \
            -DBUILD_DEC=OFF \
            "${cmakedir}"
        status=$?

        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** CMake failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
            return
        fi
    fi


    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** compile ${srcdir} ***\n${NC}"
    printf "${ORANGE}*** Make Args : ${makeargs} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    PYTHONPATH=$PYTHONPATH \
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
    LIBRARY_PATH=$LIBRARY_PATH \
      make -j$(nproc) ${makeargs}
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** Make failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        return
    fi
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** install ${srcdir} ***\n${NC}"
    printf "${ORANGE}*** Make Args : ${makeargs} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    PYTHONPATH=$PYTHONPATH \
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
    LIBRARY_PATH=$LIBRARY_PATH \
      make -j$(nproc) ${makeargs} install #exec_prefix="$HOME/bin"
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** Make failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        checkWithUser
        return
    fi

    build_time=$(( SECONDS - build_start ))
    displaytime $build_time
    
    checkWithUser

    cd ..
}

buildMeson() {
    local srcdir mesonargs prefix setuppatch bindir datarootdir datadir
    local "${@}"

    build_start=$SECONDS
    if [ $SKIP -eq 1 ]
    then
        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** Skipping Meson ${srcdir} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
        return
    fi

    datarootdir_val=""
    if [ ! -z "${datarootdir}" ]; then
        datarootdir_val="--datarootdir=${datarootdir}"
    fi

    datadir_val=""
    if [ ! -z "${datadir}" ]; then
        datadir_val="--datadir=${datadir}"
    fi

    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}* Building Github Project ***\n${NC}"
    printf "${ORANGE}* Src dir : ${srcdir} ***\n${NC}"
    printf "${ORANGE}* Prefix : ${prefix} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"

    bindir_val=""
    if [ ! -z "${bindir}" ]; then
        bindir_val="--bindir=${bindir}"
    else
        bindir_val="--bindir=bin"
    fi
    #rm -rf ${srcdir}/build
    if [ ! -d "${srcdir}/build_dir" ]; then
        mkdir -p ${srcdir}/build_dir

        cd ${srcdir}
        if [ -d "./subprojects" ]; then
            printf "${ORANGE}*****************************\n${NC}"
            printf "${ORANGE}*** Download Subprojects ${srcdir} ***\n${NC}"
            printf "${ORANGE}*****************************\n${NC}"
            # C_INCLUDE_PATH="${prefix}/include" \
            # CPLUS_INCLUDE_PATH="${prefix}/include" \
            # PATH="$HOME/bin:$PATH" \
            # LIBRARY_PATH="${prefix}/lib:$LIBRARY_PATH" \
            # LD_LIBRARY_PATH="${prefix}/lib" \
            # PKG_CONFIG_PATH="${prefix}/lib/pkgconfig" \
            #     meson subprojects download
        fi

        echo "setup patch : ${setuppatch}"
        if [ ! -z "${setuppatch}" ]; then
            printf "${ORANGE}*****************************\n${NC}"
            printf "${ORANGE}*** Meson Setup Patch ${srcdir} ***\n${NC}"
            printf "${ORANGE}*** ${setuppatch} ***\n${NC}"
            printf "${ORANGE}*****************************\n${NC}"
                bash -c "${setuppatch}"
            status=$?
            if [ $status -ne 0 ]; then
                printf "${RED}*****************************\n${NC}"
                printf "${RED}*** Bash Setup failed ${srcdir} ***\n${NC}"
                printf "${RED}*****************************\n${NC}"
                checkWithUser
                return
            fi
        fi

        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** Meson Setup ${srcdir} ***\n${NC}"
        printf "${ORANGE}*** PKG_CONFIG_PATH=$PKG_CONFIG_PATH ***\n${NC}"
        printf "${ORANGE}*** datadir=${datadir_val} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"

        cd build_dir
        PYTHONPATH=$PYTHONPATH \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
        LIBRARY_PATH=$LIBRARY_PATH \
        PATH=$PATH \
            meson setup \
                ${mesonargs} \
                --default-library=both \
                --prefix=${prefix} \
                --libdir=lib \
                --includedir=include \
                --libexecdir=libexec \
                --pkg-config-path=$PKG_CONFIG_PATH \
                --buildtype=release \
                -Dwrap_mode=nofallback \
                $bindir_val \
                ${datarootdir_val} \
                ${datadir_val} ..
        status=$?
        echo "---------------------------------------------"
        echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH \ "
        echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH \ "
        echo "LIBRARY_PATH=$LIBRARY_PATH \ "
        echo "PATH=$PATH \ "
        echo "    meson setup \ "
        echo "        ${mesonargs} \ "
        echo "        --default-library=both \ "
        echo "        --prefix=${prefix} \ "
        echo "        --libdir=lib \ "
        echo "        --includedir=include \ "
        echo "        --libexecdir=libexec \ "
        echo "        $bindir_val \ "
        echo "        ${datarootdir_val} \ "
        echo "        ${datadir_val} \ "
        echo "        --pkg-config-path=$PKG_CONFIG_PATH \ "
        echo "        --buildtype=release .. "
        echo "---------------------------------------------"
        cd ..
        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** Meson Setup failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
            checkWithUser
            return
        fi
    else
        cd ${srcdir}
        if [ -d "./subprojects" ]; then
            printf "${ORANGE}*****************************\n${NC}"
            printf "${ORANGE}*** Meson Update ${srcdir} ***\n${NC}"
            printf "${ORANGE}*****************************\n${NC}"
            # C_INCLUDE_PATH="${prefix}/include" \
            # CPLUS_INCLUDE_PATH="${prefix}/include" \
            # PATH="$HOME/bin:$PATH" \
            # LIBRARY_PATH="${prefix}/lib:$LIBRARY_PATH" \
            # LD_LIBRARY_PATH="${prefix}/lib" \
            # PKG_CONFIG_PATH="${prefix}/lib/pkgconfig" \
            #     meson subprojects update
        fi

        if [ ! -z "${setuppatch}" ]; then
            printf "${ORANGE}*****************************\n${NC}"
            printf "${ORANGE}*** Meson Setup Patch ${srcdir} ***\n${NC}"
            printf "${ORANGE}*** ${setuppatch} ***\n${NC}"
            printf "${ORANGE}*****************************\n${NC}"
                bash -c "${setuppatch}"
            status=$?
            if [ $status -ne 0 ]; then
                printf "${RED}*****************************\n${NC}"
                printf "${RED}*** Bash Setup failed ${srcdir} ***\n${NC}"
                printf "${RED}*****************************\n${NC}"
                checkWithUser
                return
            fi
        fi

        printf "${ORANGE}*****************************\n${NC}"
        printf "${ORANGE}*** Meson Reconfigure ${srcdir} ***\n${NC}"
        printf "${ORANGE}*****************************\n${NC}"
        rm -rf build_dir #Cmake state somehow gets messed up
        mkdir build_dir
        cd build_dir
        PYTHONPATH=$PYTHONPATH \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
        LIBRARY_PATH=$LIBRARY_PATH \
        PATH=$PATH \
            meson setup \
                ${mesonargs} \
                --default-library=both \
                --prefix=${prefix} \
                --libdir=lib \
                --includedir=include \
                --libexecdir=libexec \
                --pkg-config-path=$PKG_CONFIG_PATH \
                --buildtype=release \
                -Dwrap_mode=nofallback \
                $bindir_val \
                ${datarootdir_val} \
                ${datadir_val} ..
                #--reconfigure

        status=$?
        echo "---------------------------------------------"
        echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH \ "
        echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH \ "
        echo "LIBRARY_PATH=$LIBRARY_PATH \ "
        echo "PATH=$PATH \ "
        echo "    meson setup \ "
        echo "        ${mesonargs} \ "
        echo "        --default-library=both \ "
        echo "        --prefix=${prefix} \ "
        echo "        --libdir=lib \ "
        echo "        --includedir=include \ "
        echo "        --libexecdir=libexec \ "
        echo "        $bindir_val \ "
        echo "        ${datarootdir_val} \ "
        echo "        ${datadir_val} \ "
        echo "        --pkg-config-path=$PKG_CONFIG_PATH \ "
        echo "        --buildtype=release .. "
        echo "---------------------------------------------"
        cd ..
        if [ $status -ne 0 ]; then
            printf "${RED}*****************************\n${NC}"
            printf "${RED}*** Meson Setup failed ${srcdir} ***\n${NC}"
            printf "${RED}*****************************\n${NC}"
            checkWithUser
            return
        fi
    fi

    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** Meson Compile ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    PYTHONPATH=$PYTHONPATH \
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
    LIBRARY_PATH=$LIBRARY_PATH \
    PATH=$PATH \
      meson compile -C build_dir
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** Meson Compile failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        checkWithUser
        return
    fi

    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** Meson Install ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    PYTHONPATH=$PYTHONPATH \
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
    LIBRARY_PATH=$LIBRARY_PATH \
    PATH=$PATH \
      meson install -C build_dir
    status=$?

    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** Meson Install failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        checkWithUser
        return
    fi

    build_time=$(( SECONDS - build_start ))
    displaytime $build_time
    
    checkWithUser

    cd ..
}