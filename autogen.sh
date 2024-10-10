#!/bin/bash
ENABLE_LATEST=0   #Force using latest gstreamer build and static link.    enabled using --enable-latest
ENABLE_LIBAV=0    #Enables building and linking Gstreamer libav plugin.   enabled using --enable-libav
ENABLE_NVCODEC=0  #Enables building and linking Gstreamer nvidia pluging. enabled using --enable-nvcodec
ENABLE_DEBUG=1    #Disables debug build flag.                                   disabled using --no-debug
NO_DOWNLOAD=0

#Save current working directory to run configure in
WORK_DIR=$(pwd)

#Get project root directory based on autogen.sh file location
SCRT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
SUBPROJECT_DIR=$SCRT_DIR/subprojects

#Cache folder for downloaded sources
SRC_CACHE_DIR=$SUBPROJECT_DIR/.cache

#meson utility location set for pip source
MESON_TOOL=meson

#Failure marker
FAILED=0

i=1;

for arg in "$@" 
do
    shift
    if [ "$arg" == "--enable-libav=yes" ] || [ "$arg" == "--enable-libav=true" ] || [ "$arg" == "--enable-libav" ]; then
        ENABLE_LIBAV=1
        set -- "$@" "$arg"
    elif [ "$arg" == "--enable-latest" ]; then
        ENABLE_LATEST=1
        set -- "$@" "$arg"
    elif [ "$arg" == "--no-debug" ]; then
        ENABLE_DEBUG=0
    elif [ "$arg" == "--enable-nvcodec=yes" ] || [ "$arg" == "--enable-nvcodec=true" ] || [ "$arg" == "--enable-nvcodec" ]; then
        ENABLE_NVCODEC=1
        set -- "$@" "$arg"
    elif [ "$arg" == "--no-download" ]; then
        NO_DOWNLOAD=1
    else
        set -- "$@" "$arg"
    fi
    i=$((i + 1));
done

#Set debug flag on current project
if [ $ENABLE_DEBUG -eq 1 ]; then
  set -- "$@" "--enable-debug=yes"
fi

# Define color code constants
ORANGE='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

############################################
#
# Function to print time for human
#
############################################
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

printMessage(){
  local msg color padding
  local "${@}"

  if [[ -z "${padding}"  || ! "${padding}" == ?(-)+([[:digit:]]) ]]; then padding=1; fi
  paddingstr=""
  for i in $(seq 0 $padding); do paddingstr=$paddingstr" "; done

  while IFS='\n' read -r line; do
      printf "${color}***${paddingstr}${line}\n${NC}"
  done <<< "${msg}"
}

printNotice(){
  local msg
  local "${@}"
  printf "${ORANGE}*****************************\n${NC}"
  printMessage msg="${msg}" color=${ORANGE}
  printf "${ORANGE}*****************************\n${NC}"
}

printError(){
  local msg
  local "${@}"

  printf "${RED}*****************************\n${NC}"
  printMessage msg="${msg}"$'\n' color=${RED}
  printMessage msg="Kernel: $(uname -r)" color=${RED} padding=4
  printMessage msg="Kernel: $(uname -i)" color=${RED} padding=4
  printMessage msg="$(lsb_release -a 2> /dev/null)" color=${RED} padding=4
  printf "${RED}*****************************\n${NC}"
}

############################################
#
# Function to download and extract tar package file
# Can optionally use a caching folder defined with SRC_CACHE_DIR
#
############################################
downloadAndExtract (){
  local path file forcedownload # reset first
  local "${@}"

  if [ -z "${forcedownload}" ] && [ $NO_DOWNLOAD -eq 1 ]; then
    printError msg="Download disabled. Missing dependency $file"
    FAILED=1
    return;
  fi

  dest_val=""
  if [ ! -z "$SRC_CACHE_DIR" ]; then
    dest_val="$SRC_CACHE_DIR/${file}"
  else
    dest_val=${file}
  fi

  if [ ! -f "$dest_val" ]; then
    printNotice msg="Downloading : ${path}"$'\n'"Destination : $dest_val"
    wget ${path} -O $dest_val
  else
    printNotice msg="Source already downloaded : ${path}"$'\n'"Destination : $dest_val"
  fi

  printNotice msg="Extracting : ${file}"
  if [[ $dest_val == *.tar.gz ]]; then
    tar xfz $dest_val
  elif [[ $dest_val == *.tar.xz ]]; then
    tar xf $dest_val
  elif [[ $dest_val == *.tar.bz2 ]]; then
    tar xjf $dest_val
  elif [[ $dest_val == *.zip ]]; then
    unzip -o $dest_val
  else
    printError msg="Downloaded file not found. ${path} // ${file} // $dest_val"
    FAILED=1
  fi
}

############################################
#
# Function to clone a git repository or pull if it already exists locally
# Can optionally use a caching folder defined with SRC_CACHE_DIR
#
############################################
pullOrClone (){
  local path tag depth recurse ignorecache forcedownload # reset first
  local "${@}"

  if [ -z "${forcedownload}" ] && [ $NO_DOWNLOAD -eq 1 ]; then
    printError msg="Download disabled. Missing dependency $path"
    FAILED=1
    return;
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

  printNotice msg="Cloning ${tag}@${path}"
  IFS='/' read -ra ADDR <<< "$path"
  namedotgit=${ADDR[-1]}
  IFS='.' read -ra ADDR <<< "$namedotgit"
  name=${ADDR[0]}

  dest_val=""
  if [ ! -z "$SRC_CACHE_DIR" ] && [ -z "${ignorecache}" ]; then
    dest_val="$SRC_CACHE_DIR/$name"
  else
    dest_val=$name
  fi
  if [ ! -z "${tag}" ]; then
    dest_val+="-${tag}"
  fi

  if [ -z "$SRC_CACHE_DIR" ] || [ -z "${ignorecache}" ]; then 
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

  if [ ! -z "$SRC_CACHE_DIR" ] && [ -z "${ignorecache}" ]; then
    printNotice msg="Copy repo from cache"$'\n'"$dest_val"
    rm -rf $name
    cp -r $dest_val ./$name
  fi
}

############################################
#
# Function to build a project configured with autotools
#
############################################
buildMakeProject(){
  local srcdir prefix autogen autoreconf configure make cmakedir cmakeargs cmakeclean installargs skipbootstrap bootstrap configcustom outoftree
  local "${@}"

  build_start=$SECONDS

  printNotice msg="Building Project"$'\n'"Src dir : ${srcdir}"$'\n'"Prefix : ${prefix}"

  curr_dir=$(pwd)
  cd ${srcdir}

  rel_path="."
  if [ "${outoftree}" == "true" ] 
  then
    mkdir -p build
    cd build
    rel_path=".."
  fi

  if [ -f "$rel_path/bootstrap" ] && [ -z "${skipbootstrap}" ]; then
    printNotice msg="bootstrap ${srcdir}"
    $rel_path/bootstrap ${bootstrap}
    status=$?
    if [ $status -ne 0 ]; then
      printError msg="$rel_path/bootstrap failed ${srcdir}"
      FAILED=1
      cd $curr_dir
      return
    fi
  fi

  if [ -f "$rel_path/bootstrap.sh" ]; then
    printNotice msg="bootstrap.sh ${srcdir}"
    $rel_path/bootstrap.sh ${bootstrap}
    status=$?
    if [ $status -ne 0 ]; then
      printError msg="$rel_path/bootstrap.sh failed ${srcdir}"
      FAILED=1
      cd $curr_dir
      return
    fi
  fi
  if [ -f "$rel_path/autogen.sh" ] && [ "${autogen}" != "skip" ]; then
    printNotice msg="autogen ${srcdir}"
    $rel_path/autogen.sh ${autogen}
    status=$?
    if [ $status -ne 0 ]; then
      printError msg="Autogen failed ${srcdir}"
      FAILED=1
      cd $curr_dir
      return
    fi
  fi

  if [ ! -z "${autoreconf}" ] 
  then
    printNotice msg="autoreconf ${srcdir}"
    autoreconf ${autoreconf}
    status=$?
    if [ $status -ne 0 ]; then
      printError msg="Autoreconf failed ${srcdir}"
      FAILED=1
      cd $curr_dir
      return
    fi
  fi
  if [ ! -z "${cmakedir}" ] 
  then
    printNotice msg="cmake ${srcdir}"$'\n'"Args ${cmakeargs}"
    if [ ! -z "${cmakeclean}" ]
    then 
      cmake --build "${cmakedir}" --target clean
      find . -iwholename '*cmake*' -not -name CMakeLists.txt -delete
    fi

    btype="Release"
    if [ $ENABLE_DEBUG -eq 1 ]; then
      btype="Debug"
    fi
    cmake -G "Unix Makefiles" \
      ${cmakeargs} \
      -DCMAKE_BUILD_TYPE=$btype \
      -DCMAKE_INSTALL_PREFIX="${prefix}" \
      -DENABLE_TESTS=OFF \
      -DENABLE_SHARED=on \
      "${cmakedir}"
    status=$?
    if [ $status -ne 0 ]; then
      printError msg="CMake failed ${srcdir}"
      FAILED=1
      cd $curr_dir
      return
    fi
  fi

  if [ ! -z "${configcustom}" ]; then
    printNotice msg="custom config ${srcdir}"$'\n'"${configcustom}"
    bash -c "${configcustom}"
    status=$?
    if [ $status -ne 0 ]; then
      printError msg="Custom Config failed ${srcdir}"
      FAILED=1
      cd $curr_dir
      return
    fi
  fi

  if [ -f "$rel_path/configure" ] && [ -z "${cmakedir}" ]; then
    printNotice msg="configure ${srcdir}"$'\n'"Args : ${configure}"
    $rel_path/configure \
        --prefix=${prefix} \
        ${configure}
    status=$?
    if [ $status -ne 0 ]; then
      printError msg="$rel_path/configure failed ${srcdir}"
      FAILED=1
      cd $curr_dir
      return
    fi
  else
    printNotice msg="no configuration available ${srcdir}"
  fi

  printNotice msg="compile ${srcdir}"$'\n'"Args : ${makeargs}"
  make -j$(nproc) ${make}
  status=$?
  if [ $status -ne 0 ]; then
    printError msg="Make failed ${srcdir}"
    FAILED=1
    cd $curr_dir
    return
  fi

  printNotice msg="install ${srcdir}"$'\n'"Args : ${installargs}"
  make -j$(nproc) ${make} install ${installargs}
  status=$?
  if [ $status -ne 0 ]; then
    printError msg="Make failed ${srcdir}"
    FAILED=1
    cd $curr_dir
    return
  fi

  build_time=$(( SECONDS - build_start ))
  displaytime $build_time

  cd $curr_dir
}

############################################
#
# Function to build a project configured with meson
#
############################################
buildMesonProject() {
  local srcdir mesonargs prefix setuppatch bindir destdir builddir defaultlib clean
  local "${@}"

  build_start=$SECONDS

  printNotice msg="Building Project"$'\n'"Src dir : ${srcdir}"$'\n'"Prefix : ${prefix}"

  curr_dir=$(pwd)

  build_dir=""
  if [ ! -z "${builddir}" ] 
  then
    build_dir="${builddir}"
  else
    build_dir="build_dir"
  fi

  default_lib=""
  if [ ! -z "${defaultlib}" ] 
  then
    default_lib="${defaultlib}"
  else
    default_lib="static"
  fi

  bindir_val=""
  if [ ! -z "${bindir}" ]; then
      bindir_val="--bindir=${bindir}"
  fi
  
  if [ ! -z "${clean}" ]; then
    rm -rf ${srcdir}/$build_dir
  fi

  if [ ! -d "${srcdir}/$build_dir" ]; then
      mkdir -p ${srcdir}/$build_dir

      cd ${srcdir}
      if [ -d "./subprojects" ]; then
        printNotice msg="Download Subprojects ${srcdir}"
        #     $MESON_TOOL subprojects download
      fi

      echo "setup patch : ${setuppatch}"
      if [ ! -z "${setuppatch}" ]; then
        printNotice msg="Meson Setup Patch ${srcdir}"$'\n'"${setuppatch}"
        bash -c "${setuppatch}"
        status=$?
        if [ $status -ne 0 ]; then
          printError msg="Bash Setup failed ${srcdir}"
          FAILED=1
          cd $curr_dir
          return
        fi
      fi
      btype="release"
      if [ $ENABLE_DEBUG -eq 1 ]; then
        btype="debug"
      fi
      printNotice msg="Meson Setup ${srcdir}"
      $MESON_TOOL setup $build_dir \
          ${mesonargs} \
          --default-library=$default_lib \
          --prefix=${prefix} \
          $bindir_val \
          --libdir=lib \
          --includedir=include \
          --buildtype=$btype 
      status=$?
      if [ $status -ne 0 ]; then
        printError msg="Meson Setup failed ${srcdir}"
        rm -rf $build_dir
        FAILED=1
        cd $curr_dir
        return
      fi
  else
      cd ${srcdir}
      if [ -d "./subprojects" ]; then
        printNotice msg="Meson Update ${srcdir}"
        #     $MESON_TOOL subprojects update
      fi

      if [ ! -z "${setuppatch}" ]; then
        printNotice msg="Meson Setup Patch ${srcdir}"$'\n'"${setuppatch}"
          bash -c "${setuppatch}"
          status=$?
          if [ $status -ne 0 ]; then
            printError msg="Bash Setup failed ${srcdir}"
            FAILED=1
            cd $curr_dir
            return
          fi
      fi
      btype="release"
      if [ $ENABLE_DEBUG -eq 1 ]; then
        btype="debug"
      fi
      printNotice msg="Meson Reconfigure $(pwd) ${srcdir}"
      $MESON_TOOL setup $build_dir \
          ${mesonargs} \
          --default-library=$default_lib \
          --prefix=${prefix} \
          $bindir_val \
          --libdir=lib \
          --includedir=include \
          --buildtype=$btype \
          --reconfigure

      status=$?
      if [ $status -ne 0 ]; then
        printError msg="Meson Setup failed ${srcdir}"
        rm -rf $build_dir
        FAILED=1
        cd $curr_dir
        return
      fi
  fi

  printNotice msg="Meson Compile ${srcdir}"
  $MESON_TOOL compile -C $build_dir
  status=$?
  if [ $status -ne 0 ]; then
    printError msg="meson compile failed ${srcdir}"
    FAILED=1
    cd $curr_dir
    return
  fi

  printNotice msg="Meson Install ${srcdir}"
  DESTDIR=${destdir} $MESON_TOOL install -C $build_dir
  status=$?
  if [ $status -ne 0 ]; then
    printError msg="Meson Install failed ${srcdir}"
    FAILED=1
    cd $curr_dir
    return
  fi

  build_time=$(( SECONDS - build_start ))
  displaytime $build_time
  cd $curr_dir

}

pkgCheck() {
  local name minver
  local "${@}"

  min_ver=""
  if [ ! -z ${minver} ]; then
    min_ver=" >= ${minver}"
  fi
  echo $(pkg-config --print-errors --errors-to-stdout "${name} $min_ver");
}

progVersionCheck(){
  local major minor micro linenumber lineindex program paramoverride
  local "${@}"

  varparam="--version"
  if [ ! -z ${paramoverride} ]; then 
    varparam="${paramoverride}"
  fi;
  # echo "------------------------" 1> /dev/stdin
  # echo "test ${program} ${varparam}" 1> /dev/stdin
  # echo "------------------------" 1> /dev/stdin

  test=$(${program} $varparam 2> /dev/null)
  ret=$?
  if [[ $ret -ne 0 ]]; then
      echo "${program} not installed or not function $test"
      return
  fi

  FIRSTLINE=$(${program} ${varparam} | head -${linenumber})
  firstLineArray=(${FIRSTLINE//;/ })
  versionString=${firstLineArray[${lineindex}]};
  versionArray=(${versionString//./ })
  actualmajor=${versionArray[0]}
  actualminor=${versionArray[1]}
  actualmicro=${versionArray[2]}

  if [ -z "$major"  ]; then return; fi
  if [[ -z "$actualmajor"  || ! "$actualmajor" == ?(-)+([[:digit:]]) ]]; then echo "Unexpected ${program} version ouput[1] \"$FIRSTLINE\" \"$versionString\""; return; fi
  if [ "$actualmajor" -gt "$major" ]; then
    return;
  elif [ "$actualmajor" -lt "$major" ]; then
    echo "${program} version is too old[1]. $actualmajor.$actualminor.$actualmicro < $major.$minor.$micro"
  else
    if [ -z "$minor"  ]; then return; fi
    if [[ -z "$actualminor"  || ! "$actualminor" == ?(-)+([[:digit:]]) ]]; then echo "Unexpected ${program} version ouput[2] \"$FIRSTLINE\" \"$versionString\""; return; fi
    if [ "$actualminor" -gt "$minor" ]; then
      return;
    elif [ "$actualminor" -lt "$minor" ]; then
      echo "${program} version is too old[2]. $actualmajor.$actualminor.$actualmicro < $major.$minor.$micro"
    else
      if [ -z "$micro"  ]; then return; fi
      if [[ -z "$actualmicro"  || ! "$actualmicro" == ?(-)+([[:digit:]]) ]]; then echo "Unexpected ${program} version ouput[3] \"$FIRSTLINE\" \"$versionString\""; return; fi
      if [ ! -z "$micro" ] && [ "$actualmicro" -lt "$micro" ]; then
        echo "${program} version is too old[3]. $actualmajor.$actualminor.$actualmicro < $major.$minor.$micro"
      else
        return
      fi
    fi
  fi
}

buildGetTextAndAutoPoint(){
  export PATH=$PATH:$SUBPROJECT_DIR/gettext-0.21.1/dist/bin
  if [ ! -z "$(progVersionCheck program=gettextize linenumber=1 lineindex=3 major=0 minor=9 micro=18 )" ] || 
     [ ! -z "$(progVersionCheck program=autopoint linenumber=1 lineindex=3 major=0 minor=9 micro=18 )" ]; then
    echo "not found gettext"
    downloadAndExtract file="gettext-0.21.1.tar.gz" path="https://ftp.gnu.org/pub/gnu/gettext/gettext-0.21.1.tar.gz"
    if [ $FAILED -eq 1 ]; then exit 1; fi
    buildMakeProject srcdir="gettext-0.21.1" prefix="$SUBPROJECT_DIR/gettext-0.21.1/dist" autogen="skip"
    if [ $FAILED -eq 1 ]; then exit 1; fi
  else
    echo "gettext already found."
  fi
}

checkGitRepoState(){
  local repo package
  local "${@}"
  
  git -C ${repo} remote update 2> /dev/null 1> /dev/null
  LOCAL=$(git -C ${repo} rev-parse @)
  REMOTE=$(git -C ${repo} rev-parse @{u})
  BASE=$(git -C ${repo} merge-base @ @{u})
  if [ $LOCAL = $REMOTE ]; then
    echo "${repo} is already up-to-date. Do nothing..." 1> /dev/stdin
  elif [ $LOCAL = $BASE ]; then
    echo "${repo} has new changes. Force rebuild..." 1> /dev/stdin
    echo 1
    return;
  elif [ $REMOTE = $BASE ]; then
    echo "${repo} has local changes. Doing nothing..." 1> /dev/stdin
  else
    echo "Error ${repo} is diverged." 1> /dev/stdin
    echo -1
    return;
  fi

  if [ ! -z "${package}" ] && [ ! -z "$(pkgCheck name=${package})" ]; then
    echo "Package ${package} found" 1> /dev/stdin
    echo 1
    return;
  fi

  echo 0
}

# Hard dependency check
MISSING_DEP=0
if [ ! -z "$(progVersionCheck program='make')" ]; then
  MISSING_DEP=1
  echo "make build utility not found! Aborting..."
fi

if [ ! -z "$(progVersionCheck program=pkg-config)" ]; then
  MISSING_DEP=1
  echo "pkg-config build utility not found! Aborting..."
fi

if [ ! -z "$(progVersionCheck program=g++)" ]; then
  MISSING_DEP=1
  echo "g++ build utility not found! Aborting..."
fi

if [ ! -z "$(pkgCheck name=gtk+-3.0)" ]; then
  MISSING_DEP=1
  echo "libgtk-3-dev build utility not found! Aborting..."
fi

if [ $MISSING_DEP -eq 1 ]; then
  exit 1
fi

mkdir -p $SUBPROJECT_DIR
mkdir -p $SRC_CACHE_DIR

cd $SUBPROJECT_DIR

export ACLOCAL_PATH="$ACLOCAL_PATH:/usr/share/aclocal" #When using locally built autoconf, we need to add system path
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$(pkg-config --variable pc_path pkg-config); #When using locally built pkgconf, we need to add system path

#Setup gnu tools
# export PATH=$SUBPROJECT_DIR/make-4.4.1/build/dist/bin:$PATH
# if [ ! -z "$(progVersionCheck program=make linenumber=1 lineindex=2 major=4 minor=4 micro=1 )" ]; then
#   echo "building make from source..."
#   downloadAndExtract file="make-4.4.1.tar.gz" path="https://ftp.gnu.org/gnu/make/make-4.4.1.tar.gz"
#   if [ $FAILED -eq 1 ]; then exit 1; fi
#   buildMakeProject srcdir="make-4.4.1" prefix="$SUBPROJECT_DIR/make-4.4.1/build/dist" skipbootstrap="true"
#   if [ $FAILED -eq 1 ]; then exit 1; fi
# else
#   echo "make already installed."
# fi

export PATH=$SUBPROJECT_DIR/m4-1.4.19/build/dist/bin:$PATH
if [ ! -z "$(progVersionCheck program=m4 linenumber=1 lineindex=3 major=1 minor=4 micro=18 )" ]; then
  echo "building m4 from source..."
  downloadAndExtract file="m4-1.4.19.tar.xz" path="https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.xz"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="m4-1.4.19" prefix="$SUBPROJECT_DIR/m4-1.4.19/build/dist" skipbootstrap="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "m4 already installed."
fi

export PATH=$SUBPROJECT_DIR/autoconf-2.72/build/dist/bin:$PATH
export ACLOCAL_PATH="$SUBPROJECT_DIR/autoconf-2.72/build/dist/share/autoconf:$ACLOCAL_PATH"
if [ ! -z "$(progVersionCheck program=autoconf linenumber=1 lineindex=3 major=2 minor=70)" ]; then
  echo "building autoconf from source..."
  downloadAndExtract file="autoconf-2.72.tar.xz" path="https://ftp.gnu.org/gnu/autoconf/autoconf-2.72.tar.xz"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="autoconf-2.72" prefix="$SUBPROJECT_DIR/autoconf-2.72/build/dist" skipbootstrap="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "autoconf already installed."
fi

export PATH=$SUBPROJECT_DIR/automake-1.17/build/dist/bin:$PATH
export ACLOCAL_PATH="$SUBPROJECT_DIR/automake-1.17/build/dist/share/aclocal-1.17:$ACLOCAL_PATH"
if [ ! -z "$(progVersionCheck program=automake linenumber=1 lineindex=3 major=1 minor=16 micro=1)" ]; then
  echo "building automake from source..."
  downloadAndExtract file="automake-1.17.tar.xz" path="https://ftp.gnu.org/gnu/automake/automake-1.17.tar.xz"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="automake-1.17" prefix="$SUBPROJECT_DIR/automake-1.17/build/dist" skipbootstrap="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "automake already installed."
fi

export PATH=$SUBPROJECT_DIR/libtool-2.5.3/build/dist/bin:$PATH
export ACLOCAL_PATH="$SUBPROJECT_DIR/libtool-2.5.3/build/dist/share/aclocal:$ACLOCAL_PATH"
if [ ! -z "$(progVersionCheck program=libtoolize linenumber=1 lineindex=3 major=2 minor=4 micro=6)" ]; then
  echo "building libtool from source..."
  downloadAndExtract file="libtool-2.5.3.tar.xz" path="https://ftp.gnu.org/gnu/libtool/libtool-2.5.3.tar.xz"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="libtool-2.5.3" prefix="$SUBPROJECT_DIR/libtool-2.5.3/build/dist" skipbootstrap="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "libtool already installed."
fi

export PATH=$SUBPROJECT_DIR/flex-2.6.4/build/dist/bin:$PATH
if [ ! -z "$(progVersionCheck program=flex linenumber=1 lineindex=1 major=2 minor=6 micro=4 )" ]; then
  buildGetTextAndAutoPoint
  echo "building flex from source..."
  downloadAndExtract file="flex-2.6.4.tar.gz" path="https://github.com/westes/flex/releases/download/v2.6.4/flex-2.6.4.tar.gz"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="flex-2.6.4" prefix="$SUBPROJECT_DIR/flex-2.6.4/build/dist" skipbootstrap="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "flex already installed."
fi

export PATH=$SUBPROJECT_DIR/bison-3.8.2/build/dist/bin:$PATH
if [ ! -z "$(progVersionCheck program=bison linenumber=1 lineindex=3 major=3 minor=5 micro=1 )" ]; then
  echo "building bison from source..."
  downloadAndExtract file="bison-3.8.2.tar.xz" path="https://ftp.gnu.org/gnu/bison/bison-3.8.2.tar.xz"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="bison-3.8.2" prefix="$SUBPROJECT_DIR/bison-3.8.2/build/dist" skipbootstrap="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "bison already installed."
fi

#Download virtualenv tool
if [ $NO_DOWNLOAD -eq 0 ] && [ ! -f "virtualenv.pyz" ]; then
  wget https://bootstrap.pypa.io/virtualenv.pyz
fi

#Setting up Virtual Python environment
if [ ! -f "./venvfolder/bin/activate" ]; then
  python3 virtualenv.pyz ./venvfolder
fi

#Activate virtual environment
if [ -f "./venvfolder/bin/activate" ]; then
  source venvfolder/bin/activate
fi

#Setup meson
if [ $NO_DOWNLOAD -eq 0 ]; then
  if [ ! -z "$(progVersionCheck program=meson linenumber=1 lineindex=0 major=0 minor=63 micro=2)" ]; then
    echo "ninja build utility not found. Installing from pip..."
    python3 -m pip install meson --upgrade
    ret=$?
    if [ $ret != 0 ]; then
      echo "Failed to install meson via pip. Please try to install meson manually."
      exit 1
    fi
  else
    echo "Meson $($MESON_TOOL --version) already installed."
  fi

  if [ ! -z "$(progVersionCheck program=ninja)" ]; then
    echo "ninja build utility not found. Installing from pip..."
    python3 -m pip install ninja --upgrade
    ret=$?
    if [ $ret != 0 ]; then
      echo "Failed to install ninja via pip. Please try to install ninja manually."
      exit 1
    fi
  else
    echo "Ninja $(ninja --version) already installed."
  fi
else
  echo "Skipping pip install. NO_DOWNLOAD is set."
fi

# pullOrClone path="https://gitlab.gnome.org/GNOME/gtk.git" tag="3.24.34"
# buildMesonProject srcdir="gtk" prefix="$SUBPROJECT_DIR/gtk/dist" mesonargs="-Dtests=false -Dintrospection=false -Ddemos=false -Dexamples=false -Dlibepoxy:tests=false"
# if [ $FAILED -eq 1 ]; then exit 1; fi

################################################################
# 
#    Build unzip for gsoap
#       
################################################################
export PATH=$SUBPROJECT_DIR/unzip60/build/dist/bin:$PATH
if [ ! -z "$(progVersionCheck program=unzip paramoverride="-v")" ]; then
  downloadAndExtract file="unzip60.tar.gz" path="https://sourceforge.net/projects/infozip/files/UnZip%206.x%20%28latest%29/UnZip%206.0/unzip60.tar.gz/download"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="unzip60" make="-f unix/Makefile generic" installargs="prefix=$SUBPROJECT_DIR/unzip60/build/dist MANDIR=$SUBPROJECT_DIR/unzip60/build/dist/share/man/man1 -f unix/Makefile"
  if [ $FAILED -eq 1 ]; then exit 1; fi
fi

################################################################
# 
#    Build Openssl for gsoap and onvifsoap
#       
################################################################
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/openssl/build/dist/lib/pkgconfig
if [ ! -z "$(pkgCheck name=libcrypto minver=1.1.1f)" ]; then
  pullOrClone path="https://github.com/openssl/openssl.git" tag="OpenSSL_1_1_1w"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="openssl" configcustom="./config --prefix=$SUBPROJECT_DIR/openssl/build/dist --openssldir=$SUBPROJECT_DIR/openssl/build/dist/ssl -static"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "openssl already found."
fi

export PATH=$PATH:$SUBPROJECT_DIR/cmake-3.25.2/build/dist/bin
if [ ! -z "$(progVersionCheck program=cmake linenumber=1 lineindex=2 major=3 minor=16 micro=3 )" ]; then
  downloadAndExtract file="cmake-3.25.2.tar.gz" path="https://github.com/Kitware/CMake/releases/download/v3.25.2/cmake-3.25.2.tar.gz"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="cmake-3.25.2" prefix="$SUBPROJECT_DIR/cmake-3.25.2/build/dist" skipbootstrap="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "cmake already found."
fi

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/zlib-1.2.13/build/dist/lib/pkgconfig
if [ ! -z "$(pkgCheck name=zlib minver=1.2.11)" ]; then
  downloadAndExtract file="zlib-1.2.13.tar.gz" path="https://www.zlib.net/zlib-1.2.13.tar.gz"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="zlib-1.2.13" prefix="$SUBPROJECT_DIR/zlib-1.2.13/build/dist"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "zlib already found."
fi

################################################################
# 
#    Build gSoap
#       
################################################################
gsoap_version=2.8.134
export PATH=$SUBPROJECT_DIR/gsoap-2.8/build/dist/bin:$PATH
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/gsoap-2.8/build/dist/lib/pkgconfig
if [ ! -z "$(pkgCheck name=gsoap minver=$gsoap_version)" ]; then
  SSL_PREFIX=$(pkg-config --variable=prefix openssl)
  if [ "$SSL_PREFIX" != "/usr" ]; then
    SSL_INCLUDE=$(\pkg-config --variable=includedir openssl)
    SSL_LIBS=$(pkg-config --variable=libdir openssl)
  fi
  
  ZLIB_PREFIX=$(pkg-config --variable=prefix zlib)
  if [ "$ZLIB_PREFIX" != "/usr" ]; then
    ZLIB_INCLUDE=$(pkg-config --variable=includedir zlib)
    ZLIB_LIBS=$(pkg-config --variable=libdir zlib)
  fi

  echo "-- Building gsoap libgsoap-dev --"

  inc_path=""
  if [ ! -z "$SSL_INCLUDE" ] && [ "$SSL_INCLUDE" != "/usr/include" ]; then
    if [ -z "$inc_path" ]; then
      inc_path="$SSL_INCLUDE"
    else
      inc_path="$inc_path:$SSL_INCLUDE"
    fi
  fi
  if [ ! -z "$ZLIB_INCLUDE" ] && [ "$SSL_INCLUDE" != "/usr/include" ]; then
    if [ -z "$inc_path" ]; then
      inc_path="$ZLIB_INCLUDE"
    else
      inc_path="$inc_path:$ZLIB_INCLUDE"
    fi
  fi
  if [ ! -z "$CPLUS_INCLUDE_PATH" ] && [ "$SSL_INCLUDE" != "/usr/include" ]; then
    if [ -z "$inc_path" ]; then
      inc_path="$CPLUS_INCLUDE_PATH"
    else
      inc_path="$inc_path:$CPLUS_INCLUDE_PATH"
    fi
  fi

  echo "gSoap Include Path : $inc_path"
  rm -rf $SUBPROJECT_DIR/gsoap-2.8/ #Make sure we dont extract over an old version
  downloadAndExtract file="gsoap_$gsoap_version.zip" path="https://sourceforge.net/projects/gsoap2/files/gsoap_$gsoap_version.zip/download"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  C_INCLUDE_PATH="$inc_path" \
  CPLUS_INCLUDE_PATH="$inc_path" \
  LIBRARY_PATH="$SSL_LIBS:$ZLIB_LIBS:$LIBRARY_PATH" \
  LD_LIBRARY_PATH="$SSL_LIBS:$ZLIB_LIBS:$LD_LIBRARY_PATH" \
  LIBS='-ldl -lpthread' \
  buildMakeProject srcdir="gsoap-2.8" prefix="$SUBPROJECT_DIR/gsoap-2.8/build/dist" autogen="skip" configure="--with-openssl=/usr/lib/ssl"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "gsoap already found."
fi

################################################################
# 
#     Build glib dependency
#   sudo apt-get install libglib2.0-dev (gstreamer minimum 2.64.0)
# 
################################################################
export PATH=$PATH:$SUBPROJECT_DIR/glib-2.74.1/dist/bin
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/glib-2.74.1/dist/lib/pkgconfig
if [ ! -z "$(pkgCheck name=glib-2.0 minver=2.64.0)" ]; then
  echo "not found glib-2.0"
  downloadAndExtract file="glib-2.74.1.tar.xz" path="https://download.gnome.org/sources/glib/2.74/glib-2.74.1.tar.xz"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMesonProject srcdir="glib-2.74.1" prefix="$SUBPROJECT_DIR/glib-2.74.1/dist" mesonargs="-Dpcre2:test=false -Dpcre2:grep=false -Dxattr=false -Db_lundef=false -Dtests=false -Dglib_debug=disabled -Dglib_assert=false -Dglib_checks=false"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "glib already found."
fi

################################################################
# 
#    Build cutils
#       
################################################################
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/CUtils/build/dist/lib/pkgconfig
rebuild=$(checkGitRepoState repo="CUtils" package="cutils")

if [ $rebuild == 1 ]; then
  #OnvifSoapLib depends on it and will also require a rebuild
  rm -rf $SUBPROJECT_DIR/CUtils/build
  rm -rf $SUBPROJECT_DIR/OnvifSoapLib/build
  
  pullOrClone path=https://github.com/Quedale/CUtils.git ignorecache="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  #Clean up previous build in case of failure. This will prevent from falling back on the old version
  rm -rf $SUBPROJECT_DIR/CUtils/build/dist/*
  buildMakeProject srcdir="CUtils" prefix="$SUBPROJECT_DIR/CUtils/build/dist" cmakedir=".." outoftree=true cmakeclean=true
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "CUtils already found."
fi

################################################################
# 
#    Build OnvifSoapLib
#       
################################################################
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/OnvifSoapLib/build/dist/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/libntlm/build/dist/lib/pkgconfig
rebuild=$(checkGitRepoState repo="OnvifSoapLib" package="onvifsoap")

if [ $rebuild == 1 ]; then
  if [ ! -z "$(pkgCheck name=libntlm minver=v1.5)" ]; then
    pullOrClone path=https://gitlab.com/gsasl/libntlm.git tag=v1.7
    if [ $FAILED -eq 1 ]; then exit 1; fi
    buildMakeProject srcdir="libntlm" prefix="$SUBPROJECT_DIR/libntlm/build/dist" configure="--enable-shared=no" #TODO support shared linking
    if [ $FAILED -eq 1 ]; then exit 1; fi
  else 
    echo "libntlm already found."
  fi

  echo "-- Bootstrap OnvifSoapLib  --"
  pullOrClone path=https://github.com/Quedale/OnvifSoapLib.git ignorecache="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi

  #Clean up previous build in case of failure. This will prevent from falling back on the old version
  rm -rf $SUBPROJECT_DIR/OnvifSoapLib/build/dist/*

  nodownload=""
  if [ "$NO_DOWNLOAD" -eq 1 ]; then nodownload="--no-download"; fi

  GSOAP_SRC_DIR=$SUBPROJECT_DIR/gsoap-2.8 \
  C_INCLUDE_PATH="$(pkg-config --variable=includedir openssl):$(pkg-config --variable=includedir zlib):$C_INCLUDE_PATH" \
  buildMakeProject srcdir="OnvifSoapLib" prefix="$SUBPROJECT_DIR/OnvifSoapLib/build/dist" cmakedir=".." cmakeargs="-DGSOAP_SRC_DIR=$SUBPROJECT_DIR/gsoap-2.8" bootstrap="$nodownload --skip-gsoap $skipwsdl" outoftree=true cmakeclean=true
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "onvifsoaplib already found."
fi

################################################################
# 
#    Build alsa-lib dependency
#   sudo apt install llibasound2-dev (tested 1.2.7.2)
# 
################################################################
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/alsa-lib/build/dist/lib/pkgconfig
if [ ! -z "$(pkgCheck name=alsa minver=1.2.2)" ]; then
  echo "not found alsa"
  pullOrClone path=git://git.alsa-project.org/alsa-lib.git tag=v1.2.8
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="alsa-lib" prefix="$SUBPROJECT_DIR/alsa-lib/build/dist" configure="--enable-static=no --enable-shared=yes" autoreconf="-vif"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "alsa-lib already found."
fi

################################################################
# 
#    Build Gstreamer dependency
#       sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
################################################################
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/FFmpeg/dist/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/gstreamer/build_omx/dist/lib/gstreamer-1.0/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/gstreamer/build/dist/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig

gst_ret=0
GSTREAMER_LATEST=1.24.8
if [ $ENABLE_LATEST == 0 ]; then
  GSTREAMER_VERSION=1.14.4
else
  GSTREAMER_VERSION=$GSTREAMER_LATEST
fi

gst_core=(
  "gstreamer-1.0;gstreamer-1.0"
  "gstreamer-plugins-base-1.0;gstreamer-plugins-base-1.0"
  "gstreamer-plugins-good-1.0;gstreamer-plugins-good-1.0" #For some reason this doesn't come out? Relying on plugins
  "gstreamer-plugins-bad-1.0;gstreamer-plugins-bad-1.0"
)

gst_base_plugins=(
    "gl;gstopengl"
    "app;gstapp"
    "typefind;gsttypefindfunctions"
    "audiotestsrc;gstaudiotestsrc"
    # "videotestsrc"
    "playback;gstplayback"
    "x11;gstximagesink"
    "alsa;gstalsa"
    "videoconvertscale;gstvideoconvertscale"
    "videorate;gstvideorate"
    "rawparse;gstrawparse"
    "pbtypes;gstpbtypes"
    "audioresample;gstaudioresample"
    "audioconvert;gstaudioconvert"
    "volume;gstvolume"
    "tcp;gsttcp"
    "overlaycomposition;gstoverlaycomposition"
  )
gst_good_plugins=(
    "gtk3;gstgtk"
    "level;gstlevel"
    "rtsp;gstrtsp"
    "jpeg;gstjpeg"
    "rtp;gstrtp"
    "rtpmanager;gstrtpmanager"
    "law;gstmulaw"
    "law;gstalaw"
    "autodetect;gstautodetect"
    "pulse;gstpulseaudio"
    "interleave;gstinterleave"
    "audioparsers;gstaudioparsers"
    "udp;gstudp"
    "v4l2;gstvideo4linux2"
    # "debugutils"  # This is to support v4l2h264enc element with capssetter #Workaround https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/1056
    # "png" # This is required for the snapshot feature
)

gst_bad_plugins=(
    # "x11;gstx11"
    # "gl;gstgl"
    "interlace;gstinterlace"
    "openh264;gstopenh264"
    "fdkaac;gstfdkaac"
    "videoparsers;gstvideoparsersbad"
    "onvif;gstrtponvif"
    "jpegformat;gstjpegformat"
    "v4l2codecs;gstv4l2codecs"
    "libde265;gstde265"
)
if [ $ENABLE_NVCODEC != 0 ]; then gst_bad_plugins+=("nvcodec;gstnvcodec"); fi

checkGstreamerPkg () {
  local static version # reset first
  local "${@}"

  gstpkgret=0;
  if [ -z "${static}" ]; then
    for gst_p in ${gst_core[@]}; do
      IFS=";" read -r -a arr <<< "${gst_p}"
      if [ ! -z "$(pkgCheck name=${arr[1]} minver=${version})" ]; then
        printf "  missing core package ${arr[0]} >= ${version}\n";
      fi
    done
  else
    for gst_p in ${gst_base_plugins[@]}; do
      IFS=";" read -r -a arr <<< "${gst_p}"
      if [ ! -z "$(pkgCheck name=${arr[1]} minver=${version})" ]; then
        printf "  missing base plugin ${arr[0]} >= ${version}\n";
      fi
    done
    for gst_p in ${gst_good_plugins[@]}; do
      IFS=";" read -r -a arr <<< "${gst_p}"
      if [ ! -z "$(pkgCheck name=${arr[1]} minver=${version})" ]; then
        printf "  missing good plugin ${arr[0]} >= ${version}\n";
      fi
    done
    for gst_p in ${gst_bad_plugins[@]}; do
      IFS=";" read -r -a arr <<< "${gst_p}"
      if [ ! -z "$(pkgCheck name=${arr[1]} minver=${version})" ]; then
        printf "  missing bad plugin ${arr[0]} >= ${version}\n";
      fi
    done
  fi
}

#Gstreamer install on system doesn't break down by plugin, but groups them under base,good,bad,ugly
if [ ! -z "$(checkGstreamerPkg version=$GSTREAMER_VERSION)" ]; then
  printf "Gstreamer not installed on system.. \n$(checkGstreamerPkg version=$GSTREAMER_VERSION)\n"
  gst_ret=1;
fi

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/pulseaudio/build/dist/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/libde265/dist/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/libx11/dist/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/macros/dist/lib/pkgconfig

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/systemd-256/build/dist/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/libgudev/build/dist/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/libcap/dist/lib64/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/util-linux/dist/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SUBPROJECT_DIR/libsndfile/dist/lib/pkgconfig
export PATH=$SUBPROJECT_DIR/gperf-3.1/dist/bin:$PATH
export PATH=$SUBPROJECT_DIR/nasm-2.15.05/dist/bin:$PATH
export PATH=$SUBPROJECT_DIR/FFmpeg/dist/bin:$PATH

#Check to see if gstreamer exist on the system
if [ $gst_ret != 0 ] || [ $ENABLE_LATEST != 0 ]; then
  gst_ret=0;
  GSTREAMER_VERSION=$GSTREAMER_LATEST; #If we are to build something, build latest
  #Gstreamer static plugins needs to be checked individually
  if [ ! -z "$(checkGstreamerPkg version=$GSTREAMER_VERSION static=true)" ]; then
    printf "Gstreamer static library not built.. \n$(checkGstreamerPkg version=$GSTREAMER_VERSION static=true)\n"
    gst_ret=1;
  fi
  if [ $ENABLE_LIBAV -eq 1 ] && [ ! -z "$(pkgCheck name=gstlibav minver=$GSTREAMER_VERSION)" ]; then
    echo "missing libav plugin >= $GSTREAMER_VERSION";
    gst_ret=1;
  fi

  #Global check if gstreamer is already built
  if [ $gst_ret != 0 ]; then
    buildGetTextAndAutoPoint

    ################################################################
    # 
    #    Build gudev-1.0 dependency
    #   sudo apt install libgudev-1.0-dev (tested 232)
    # 
    ################################################################
    if [ ! -z "$(pkgCheck name=gudev-1.0 minver=232)" ]; then
      echo "not found gudev-1.0"
      if [ ! -z "$(progVersionCheck program=gperf)" ]; then
        downloadAndExtract file="gperf-3.1.tar.gz" path="http://ftp.gnu.org/pub/gnu/gperf/gperf-3.1.tar.gz"
        if [ $FAILED -eq 1 ]; then exit 1; fi
        buildMakeProject srcdir="gperf-3.1" prefix="$SUBPROJECT_DIR/gperf-3.1/dist" autogen="skip"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "gperf already found."
      fi

      if [ ! -z "$(pkgCheck name=libcap minver=2.53)" ]; then
        echo "not found libcap"
        #old link git://git.kernel.org/pub/scm/linux/kernel/git/morgan/libcap.git
        pullOrClone path=https://kernel.googlesource.com/pub/scm/libs/libcap/libcap tag=libcap-2.69
        if [ $FAILED -eq 1 ]; then exit 1; fi
        buildMakeProject srcdir="libcap" prefix="$SUBPROJECT_DIR/libcap/dist" installargs="DESTDIR=$SUBPROJECT_DIR/libcap/dist"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "libcap already found."
      fi

      #TODO build autopoint... (Mint linux)
      if [ ! -z "$(pkgCheck name=mount minver=2.34.0)" ]; then
        echo "not found mount"
        pullOrClone path=https://github.com/util-linux/util-linux.git tag=v2.38.1
        if [ $FAILED -eq 1 ]; then exit 1; fi
        buildMakeProject srcdir="util-linux" prefix="$SUBPROJECT_DIR/util-linux/dist" configure="--disable-rpath --disable-bash-completion --disable-makeinstall-setuid --disable-makeinstall-chown"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "mount already found."
      fi

      python3 -c 'from setuptools import setup'
      ret=$?
      if [ $ret != 0 ]; then
        python3 -m pip install setuptools
      else
        echo "python3 setuptools already found."
      fi

      python3 -c 'import jinja2'
      ret=$?
      if [ $ret != 0 ]; then
        python3 -m pip install jinja2
      else
        echo "python3 jinja2 already found."
      fi

      if [ ! -z "$(pkgCheck name=libudev minver=256)" ]; then
        echo "not found libudev"
        SYSD_MESON_ARGS="-Dauto_features=disabled"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dmode=developer"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-udev-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-systemctl-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-networkd-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-timesyncd-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-journalctl-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-boot-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dfirst-boot-full-preset=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dstatic-libsystemd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dstatic-libudev=true"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dstandalone-binaries=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dinitrd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcompat-mutable-uid-boundaries=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnscd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Ddebug-shell=''"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Ddebug-tty=''"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dutmp=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dhibernate=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dldconfig=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dresolve=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Defi=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtpm=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Denvironment-d=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbinfmt=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Drepart=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsysupdate=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcoredump=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dpstore=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Doomd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlogind=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dhostnamed=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlocaled=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dmachined=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dportabled=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsysext=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Duserdb=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dhomed=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnetworkd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtimedated=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtimesyncd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dremote=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcreate-log-dirs=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnss-myhostname=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnss-mymachines=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnss-resolve=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnss-systemd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Drandomseed=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbacklight=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dvconsole=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dquotacheck=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsysusers=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtmpfiles=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dimportd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dhwdb=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Drfkill=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dxdg-autostart=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dman=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dhtml=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtranslations=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dinstall-sysconfdir=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dseccomp=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dselinux=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dapparmor=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsmack=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dpolkit=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dima=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dacl=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Daudit=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dblkid=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dfdisk=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dkmod=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dpam=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dpwquality=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dmicrohttpd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibcryptsetup=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibcurl=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Didn=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibidn2=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibidn=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibiptc=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dqrencode=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dgcrypt=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dgnutls=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dopenssl=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcryptolib=auto"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dp11kit=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibfido2=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtpm2=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Delfutils=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dzlib=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbzip2=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dxz=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlz4=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dzstd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dxkbcommon=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dpcre2=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dglib=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Ddbus=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtests=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Durlify=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Danalyze=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbpf-framework=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dkernel-install=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsysvinit-path=$SUBPROJECT_DIR/systemd-256/build/dist/init.d"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbashcompletiondir=$SUBPROJECT_DIR/systemd-256/build/dist/bash-completion"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcreate-log-dirs=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlocalstatedir=$SUBPROJECT_DIR/systemd-256/build/dist/localstate"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsshconfdir=$SUBPROJECT_DIR/systemd-256/build/dist/sshcfg"
        downloadAndExtract file="v256.tar.gz" path="https://github.com/systemd/systemd/archive/refs/tags/v256.tar.gz"
        if [ $FAILED -eq 1 ]; then exit 1; fi
        C_INCLUDE_PATH=$SUBPROJECT_DIR/libcap/dist/usr/include \
        LIBRARY_PATH=$SUBPROJECT_DIR/libcap/dist/lib64 \
        buildMesonProject srcdir="systemd-256" prefix="$SUBPROJECT_DIR/systemd-256/build/dist" mesonargs="$SYSD_MESON_ARGS"
        if [ $FAILED -eq 1 ]; then exit 1; fi

      else
        echo "libudev already found."
      fi

      pullOrClone path=https://gitlab.gnome.org/GNOME/libgudev.git tag=237
      if [ $FAILED -eq 1 ]; then exit 1; fi
      C_INCLUDE_PATH=$SUBPROJECT_DIR/systemd-256/build/dist/include \
      LIBRARY_PATH=$SUBPROJECT_DIR/libcap/dist/lib64:$SUBPROJECT_DIR/systemd-256/build/dist/lib \
      buildMesonProject srcdir="libgudev" prefix="$SUBPROJECT_DIR/libgudev/build/dist" mesonargs="-Dvapi=disabled -Dtests=disabled -Dintrospection=disabled"
      if [ $FAILED -eq 1 ]; then exit 1; fi

    else
      echo "gudev-1.0 already found."
    fi

    ################################################################
    # 
    #    Build libpulse dependency
    #   sudo apt install libpulse-dev (tested 12.2)
    # 
    ################################################################
    if [ ! -z "$(pkgCheck name=libpulse minver=12.2)" ]; then
      echo "not found libpulse"

      if [ ! -z "$(pkgCheck name=sndfile minver=1.2.0)" ]; then
        echo "not found sndfile"
        LIBSNDFILE_CMAKEARGS="-DBUILD_EXAMPLES=off"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DBUILD_TESTING=off"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DBUILD_SHARED_LIBS=on"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DINSTALL_PKGCONFIG_MODULE=on"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DINSTALL_MANPAGES=off"
        pullOrClone path="https://github.com/libsndfile/libsndfile.git" tag=1.2.0
        if [ $FAILED -eq 1 ]; then exit 1; fi
        mkdir "libsndfile/build"
        buildMakeProject srcdir="libsndfile/build" prefix="$SUBPROJECT_DIR/libsndfile/dist" cmakedir=".." cmakeargs="$LIBSNDFILE_CMAKEARGS"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "sndfile already found."
      fi

      PULSE_MESON_ARGS="-Ddaemon=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Ddoxygen=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dman=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dtests=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Ddatabase=simple"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dbashcompletiondir=no"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dzshcompletiondir=no"
      pullOrClone path="https://gitlab.freedesktop.org/pulseaudio/pulseaudio.git" tag=v16.1
      if [ $FAILED -eq 1 ]; then exit 1; fi
      buildMesonProject srcdir="pulseaudio" prefix="$SUBPROJECT_DIR/pulseaudio/build/dist" mesonargs="$PULSE_MESON_ARGS"
      if [ $FAILED -eq 1 ]; then exit 1; fi

    else
      echo "libpulse already found."
    fi

    ################################################################
    # 
    #    Build nasm dependency
    #   sudo apt install nasm
    # 
    ################################################################
    if [ ! -z "$(progVersionCheck program=nasm)" ]; then
      echo "not found nasm"
      downloadAndExtract file=nasm-2.15.05.tar.bz2 path=https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.bz2
      if [ $FAILED -eq 1 ]; then exit 1; fi
      buildMakeProject srcdir="nasm-2.15.05" prefix="$SUBPROJECT_DIR/nasm-2.15.05/dist"
      if [ $FAILED -eq 1 ]; then exit 1; fi
    else
        echo "nasm already installed."
    fi

    if [ ! -z "$(pkgCheck name=libde265 minver=v1.0.15)" ]; then
      pullOrClone path="https://github.com/strukturag/libde265.git" tag=v1.0.15
      if [ $FAILED -eq 1 ]; then exit 1; fi
      buildMakeProject srcdir="libde265" prefix="$SUBPROJECT_DIR/libde265/dist" configure="--disable-sherlock265"
      if [ $FAILED -eq 1 ]; then exit 1; fi
    else
      echo "libde265 already installed."
    fi

    if [ ! -z "$(pkgCheck name=x11-xcb minver=1.7.2)" ]; then
      if [ ! -z "$(pkgCheck name=xorg-macros minver=1.19.1)" ]; then
        pullOrClone path="https://gitlab.freedesktop.org/xorg/util/macros.git" tag="util-macros-1.20.0"
        if [ $FAILED -eq 1 ]; then exit 1; fi
        buildMakeProject srcdir="macros" prefix="$SUBPROJECT_DIR/macros/dist" configure="--datarootdir=$SUBPROJECT_DIR/macros/dist/lib"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "xorg-macro already installed."
      fi

      pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libx11.git" tag="libX11-1.8.8"
      if [ $FAILED -eq 1 ]; then exit 1; fi
      ACLOCAL_PATH="$SUBPROJECT_DIR/macros/dist/lib/aclocal:$ACLOCAL_PATH" \
      buildMakeProject srcdir="libx11" prefix="$SUBPROJECT_DIR/libx11/dist"
      if [ $FAILED -eq 1 ]; then exit 1; fi
    else
      echo "x11-xcb already installed."
    fi

    MESON_PARAMS=""
    if [ $ENABLE_LIBAV -eq 1 ]; then
        echo "LIBAV Feature enabled..."
        
        [[ ! -z "$(pkgCheck name=libavcodec minver=58.20.100)" ]] && ret1=1 || ret1=0
        [[ ! -z "$(pkgCheck name=libavfilter minver=7.40.101)" ]] && ret2=1 || ret2=0
        [[ ! -z "$(pkgCheck name=libavformat minver=58.20.100)" ]] && ret3=1 || ret3=0
        [[ ! -z "$(pkgCheck name=libavutil minver=56.22.100)" ]] && ret4=1 || ret4=0
        [[ ! -z "$(pkgCheck name=libpostproc minver=55.3.100)" ]] && ret5=1 || ret5=0
        [[ ! -z "$(pkgCheck name=libswresample minver=3.3.100)" ]] && ret6=1 || ret6=0
        [[ ! -z "$(pkgCheck name=libswscale minver=5.3.100)" ]] && ret7=1 || ret7=0

        if [ $ret1 != 0 ] || [ $ret2 != 0 ] || [ $ret3 != 0 ] || [ $ret4 != 0 ] || [ $ret5 != 0 ] || [ $ret6 != 0 ] || [ $ret7 != 0 ]; then
            #######################
            #
            # Custom FFmpeg build
            #   For some reason, Gstreamer's meson dep doesn't build any codecs
            #   
            #   sudo apt install libavcodec-dev libavfilter-dev libavformat-dev libswresample-dev
            #######################
            echo "building FFmpeg from source"
            FFMPEG_CONFIGURE_ARGS="--disable-lzma"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --disable-doc"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --disable-shared"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-static"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-nonfree"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-version3"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-gpl"

            pullOrClone path="https://github.com/FFmpeg/FFmpeg.git" tag=n6.1.1
            if [ $FAILED -eq 1 ]; then exit 1; fi
            buildMakeProject srcdir="FFmpeg" prefix="$SUBPROJECT_DIR/FFmpeg/dist" configure="$FFMPEG_CONFIGURE_ARGS"
            if [ $FAILED -eq 1 ]; then exit 1; fi
            rm -rf $SUBPROJECT_DIR/FFmpeg/dist/lib/*.so
        else
            echo "FFmpeg already installed."
        fi
        MESON_PARAMS="$MESON_PARAMS -Dlibav=enabled"
    fi

    pullOrClone path="https://gitlab.freedesktop.org/gstreamer/gstreamer.git" tag=$GSTREAMER_VERSION
    if [ $FAILED -eq 1 ]; then exit 1; fi
    
    #build meson params
    # Force disable subproject features
    MESON_PARAMS="$MESON_PARAMS -Dglib:tests=false"
    MESON_PARAMS="$MESON_PARAMS -Dlibdrm:cairo-tests=false"
    MESON_PARAMS="$MESON_PARAMS -Dx264:cli=false"

    # Gstreamer options
    MESON_PARAMS="$MESON_PARAMS -Dbase=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgood=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dbad=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgpl=enabled"
    for gst_p in ${gst_base_plugins[@]}; do
      IFS=";" read -r -a arr <<< "${gst_p}"
      MESON_PARAMS+=" -Dgst-plugins-base:${arr[0]}=enabled"
    done
    for gst_p in ${gst_good_plugins[@]}; do
      IFS=";" read -r -a arr <<< "${gst_p}"
      MESON_PARAMS+=" -Dgst-plugins-good:${arr[0]}=enabled"
    done
    for gst_p in ${gst_bad_plugins[@]}; do
      IFS=";" read -r -a arr <<< "${gst_p}"
      MESON_PARAMS+=" -Dgst-plugins-bad:${arr[0]}=enabled"
    done
    MESON_PARAMS="-Dauto_features=disabled $MESON_PARAMS"
    MESON_PARAMS="--strip $MESON_PARAMS"

    # Add tinyalsa fallback subproject
    # echo "[wrap-git]" > subprojects/tinyalsa.wrap
    # echo "directory=tinyalsa" >> subprojects/tinyalsa.wrap
    # echo "url=https://github.com/tinyalsa/tinyalsa.git" >> subprojects/tinyalsa.wrap
    # echo "revision=v2.0.0" >> subprojects/tinyalsa.wrap
    # MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:tinyalsa=enabled"

    LIBRARY_PATH=$LD_LIBRARY_PATH:$SUBPROJECT_DIR/systemd-256/dist/lib \
    buildMesonProject srcdir="gstreamer" prefix="$SUBPROJECT_DIR/gstreamer/build/dist" mesonargs="$MESON_PARAMS" builddir="build"
    if [ $FAILED -eq 1 ]; then exit 1; fi
  else
      echo "Latest Gstreamer already installed/built."
  fi
else
    echo "Gstreamer already installed."
fi

printNotice msg="Bootstrap OnvifDeviceManager"
#Change to the project folder to run autoconf and automake
cd $SCRT_DIR
#Initialize project
aclocal
autoconf
automake --add-missing
autoreconf -i

configArgs=$@
printNotice msg="Configuring OnvifDeviceManager"$'\n'"Args : $configArgs"
cd $WORK_DIR
$SCRT_DIR/configure $configArgs
status=$?
if [ $status -ne 0 ]; then
  printError msg="Failed to configure OnvifDeviceManager"
else
  printNotice msg="OnvifDeviceManager is ready to be built."$'\n'"  Simply run \"make -j\$(nproc)\""
fi