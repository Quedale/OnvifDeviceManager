#!/bin/bash
ENABLE_LATEST=0   #Force using latest gstreamer build and static link.    enabled using --enable-latest
ENABLE_LIBAV=0    #Enables building and linking Gstreamer libav plugin.   enabled using --enable-libav
ENABLE_NVCODEC=0  #Enables building and linking Gstreamer nvidia pluging. enabled using --enable-nvcodec
ENABLE_DEBUG=1    #Disables debug build flag.                                   disabled using --no-debug
NO_DOWNLOAD=0
GEN_ICONS=0
WGET_CMD="wget"   #Default wget command. Gets properly initialized later with parameters adequate for its version

#Save current working directory to run configure in
WORK_DIR=$(pwd)
#Get project root directory based on autogen.sh file location
SCRT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
if [[ $SCRT_DIR = *" "* ]]; then #This is an autoconf workaround to support whitespace in $PATH
  printError project="onvifmgr" task="init" msg="Whitespace found in current working directory."$'\n'"please move the project to a location without whitespace in its path."
  exit 1
fi
SUBPROJECT_DIR=$SCRT_DIR/subprojects

#Cache folder for downloaded sources
SRC_CACHE_DIR=$SUBPROJECT_DIR/.cache

# Define color code constants
YELLOW='\033[0;33m'
BHIGREEN='\x1b[38;5;118m'
RED='\033[0;31m'
NC='\033[0m' # No Color
CYAN='\033[0;36m'
GREEN='\033[0;32m'

#Failure marker
FAILED=0

script_start=$SECONDS
og_pkg_path=$PKG_CONFIG_PATH

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
    elif [ "$arg" == "--generate-icons-only" ]; then
        GEN_ICONS=1
    else
        set -- "$@" "$arg"
    fi
    i=$((i + 1));
done

generateIcons(){
  #Compile small resizer utility
  gcc -o image-resizer $SCRT_DIR/src/image-resizer.c `pkg-config --cflags --libs gtk+-3.0` | printlines project="imgresizer" task="compile"
  [ $? -ne 0 ] && printError project="onvifmgr" task="aclocal" msg="Failed to build image-resizer" && exit 1

  #Generate application icons
  mkdir -p $SCRT_DIR/images/generated_icons/16x16 && ./image-resizer $SCRT_DIR/images/onvifmgr.png $SCRT_DIR/images/generated_icons/16x16/onvifmgr.png 16 16 | printlines project="imgresizer" task="resize"
  [ $? -ne 0 ] && printError project="imgresizer" task="resize" msg="Failed to create 16x16 icon" && exit 1
  mkdir -p $SCRT_DIR/images/generated_icons/32x32 && ./image-resizer $SCRT_DIR/images/onvifmgr.png $SCRT_DIR/images/generated_icons/32x32/onvifmgr.png 32 32 | printlines project="imgresizer" task="resize"
  [ $? -ne 0 ] && printError project="imgresizer" task="resize" msg="Failed to create 32x32 icon" && exit 1
  mkdir -p $SCRT_DIR/images/generated_icons/64x64 && ./image-resizer $SCRT_DIR/images/onvifmgr.png $SCRT_DIR/images/generated_icons/64x64/onvifmgr.png 64 64 | printlines project="imgresizer" task="resize"
  [ $? -ne 0 ] && printError project="imgresizer" task="resize" msg="Failed to create 64x64 icon" && exit 1
  mkdir -p $SCRT_DIR/images/generated_icons/128x128 && ./image-resizer $SCRT_DIR/images/onvifmgr.png $SCRT_DIR/images/generated_icons/128x128/onvifmgr.png 128 128 | printlines project="imgresizer" task="resize"
  [ $? -ne 0 ] && printError project="imgresizer" task="resize" msg="Failed to create 128x128 icon" && exit 1
  mkdir -p $SCRT_DIR/images/generated_icons/256x256 && ./image-resizer $SCRT_DIR/images/onvifmgr.png $SCRT_DIR/images/generated_icons/256x256/onvifmgr.png 256 256 | printlines project="imgresizer" task="resize"
  [ $? -ne 0 ] && printError project="imgresizer" task="resize" msg="Failed to create 256x256 icon" && exit 1
}

#Set debug flag on current project
[ $ENABLE_DEBUG -eq 1 ] && set -- "$@" "--enable-debug=yes"

unbufferCall(){
  if [ ! -z "$UNBUFFER_COMMAND" ]; then
    eval "unbuffer ${@}"
  else
    script -efq -c "$(printf "%s" "${@}")"
  fi
}

##############################
# Function to decorate lines with project, task and color
# It handles targeted ANSI and VT codes to maintain appropriate decoration
#   Codes : \r, \n, ^[[1K, ^[[2K, ^[<n>G
##############################
CSI_G_REGEX='^\[([0-9][0-9]*)G' #CSI <n> G: Cursor horizontal absolute
VT_CLEARLINE_REGEX='^\[[{1}|{2}]K' #EL1 clearbol & EL2 clearline
printline(){
  local project task color padding prefix val escape_buff
  local "${@}"
  line="" #Reset variable before using it
  escape_marker=0
  escape_buff=""
  carriage_returned=1

  while IFS= read -rn1 data; do
    if [[ "$data" == *\* ]]; then #escape codes flag
      if [ ! -z "$escape_buff" ]; then #save previous parsed escape code for later flush
        line="$line""$escape_buff"
        escape_buff=""
      fi
      escape_marker=1 #Reset marker
    elif [ -z "$data" ]; then #Newline, Flush buffer with newline
      if [ $carriage_returned -eq 1 ]; then
        printf "${GREEN}${project}${CYAN} ${task}${color}${prefix}${padding}%s%s${NC}"$'\n'$'\r' "$line" "$escape_buff" >&2
      else
        printf "%s%s"$'\n'$'\r' "$line" "$escape_buff" >&2
      fi
      escape_marker=0
      escape_buff=""
      line=""
      carriage_returned=1
    elif [ "$data" == $'\r' ]; then #Carriage return. Flush buffer
      if [ $carriage_returned -eq 1 ]; then
        printf "${GREEN}${project}${CYAN} ${task}${color}${prefix}${padding}%s%s${NC}"$'\r' "$line" "$escape_buff" >&2
      else
        printf "%s%s"$'\r' "$line" "$escape_buff" >&2
      fi
      escape_marker=0
      escape_buff=""
      line=""
      carriage_returned=1
    elif [ $escape_marker -gt 0 ]; then #Seperated the rest of the condition to save on performance
      escape_buff+=$data
      ((escape_marker++))

      if [ $escape_marker -eq 4 ] && [[ "$escape_buff" =~ $VT_CLEARLINE_REGEX ]]; then
        printf "%s%s" "$line" "$escape_buff"
        escape_marker=0
        escape_buff=""
        line=""
        carriage_returned=1
      elif  [ $escape_marker -ge 4 ] && [[ $escape_buff =~ $CSI_G_REGEX ]]; then
        val=${BASH_REMATCH[1]}
        ((val+=${#project})) #Adjust adsolute positioning after project and task label
        ((val+=${#task}))
        ((val+=${#padding}))
        ((val++))
        escape_buff="["$val"G"
        printf "%s%s" "$line" "$escape_buff" #Print without decoaration since we moved after prefix
        line=""
        escape_marker=0
        escape_buff=""
        carriage_returned=0
      elif [ $escape_marker -ge 5 ]; then #No processing of any other escape code
        escape_marker=0
        line="$line""$escape_buff"
        escape_buff=""
      else
        continue;
      fi
    elif [ ! -z "$data" ]; then #Save regular character for later flush
      line="$line""$data"
    fi
  done;

  #Flush remaining buffers
  if [ ! -z "$line" ]; then
    printf "%s" "$line"
  fi
  if [ ! -z "$escape_buff" ]; then
    printf "%s" "$escape_buff"
  fi
}

printlines(){
  local project task msg color padding prefix
  local "${@}"

  #Init default color string
  [[ ! -z "${color}" ]] && incolor="${color}" || incolor="${NC}"

  #Init padding string
  if [[ -z "${padding}"  || ! "${padding}" == ?(-)+([[:digit:]]) ]]; then padding=1; fi
  inpadding="" && for i in {1..$padding}; do inpadding=$inpadding" "; done

  if [ ! -z "${msg}" ]; then
    echo "${msg}" | printline project="${project}" task="${task}" color="${incolor}" padding="${inpadding}" prefix="${prefix}"
  else
    printline project="${project}" task="${task}" color="${incolor}" padding="${inpadding}" prefix="${prefix}"
  fi
}

printError(){
  local msg project task
  local "${@}"
  printlines project="${project}" task="${task}" color=${RED} msg="*****************************"
  printlines project="${project}" task="${task}" color=${RED} prefix=" * " msg="${msg}"
  printlines project="${project}" task="${task}" color=${RED} prefix=" * " padding=4 msg="Kernel: $(uname -r)"
  printlines project="${project}" task="${task}" color=${RED} prefix=" * " padding=4 msg="Kernel: $(uname -i)"
  #Doesnt exist on REHL
  # printlines project="${project}" task="${task}" color=${RED} prefix=" * " padding=4 msg="$(lsb_release -a 2> /dev/null)"
  printlines project="${project}" task="${task}" color=${RED} msg="*****************************"
}

printNotice(){
  local msg project task color
  local "${@}"
  [[ ! -z "${color}" ]] && incolor="${color}" || incolor="${BHIGREEN}"
  printlines project="${project}" task="${task}" color=${incolor} msg="*****************************"
  printlines project="${project}" task="${task}" color=${incolor} prefix=" * " msg="${msg}"
  printlines project="${project}" task="${task}" color=${incolor} msg="*****************************"
}

############################################
#
# Function to print time for human
#
############################################
function displaytime {
  local time project task msg label
  local "${@}"
  local T=${time}
  local D=$((T/60/60/24))
  local H=$((T/60/60%24))
  local M=$((T/60%60))
  local S=$((T%60))
  [[ ! -z "${msg}" ]] && output="${msg}"$'\n'$'\n'$label || output="$label"
  (( $D > 0 )) && output=$output"$D days "
  (( $H > 0 )) && output=$output"$H hours "
  (( $M > 0 )) && output=$output"$M minutes "
  (( $D > 0 || $H > 0 || $M > 0 )) && output=$output"and "
  output=$output"$S seconds"
  printNotice project="${project}" task="${task}" msg="$output"
}

############################################
#
# Function to download and extract tar package file
# Can optionally use a caching folder defined with SRC_CACHE_DIR
#
############################################
downloadAndExtract (){
  local project path file forcedownload noexit # reset first
  local "${@}"

  if [ -z "${forcedownload}" ] && [ $NO_DOWNLOAD -eq 1 ]; then
    printError project="${project}" task="wget" msg="download disabled. Missing dependency $file"
    [[ ! -z "${noexit}" ]] && FAILED=1 && return || exit 1;
  fi

  [ ! -z "$SRC_CACHE_DIR" ] && pathprefix="$SRC_CACHE_DIR/" || pathprefix=""
  
  dest_val=$pathprefix$file
  if [ ! -f "$dest_val" ]; then
    printlines project="${project}" task="wget" msg="downloading: ${path}"
    cd "$pathprefix" #Changing directory so that wget logs only the filename
    $WGET_CMD ${path} -O ${file} 2>&1 | printlines project="${project}" task="wget";
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="wget" msg="failed to fetch ${path}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
    cd "$OLDPWD"
  else
    printlines project="${project}" task="wget" msg="source already downloaded"
  fi

  printlines project="${project}" task="wget" msg="extracting : ${file}"
  if [[ "${pathprefix}${file}" == *.tar.gz ]]; then
    tar xfz "${pathprefix}${file}" 2>&1 | printlines project="${project}" task="extract";
  elif [[ "${pathprefix}${file}" == *.tar.xz ]]; then
    tar xf "${pathprefix}${file}" 2>&1 | printlines project="${project}" task="extract";
  elif [[ "${pathprefix}${file}" == *.tar.bz2 ]]; then
    tar xjf "${pathprefix}${file}" 2>&1 | printlines project="${project}" task="extract";
  elif [[ "${pathprefix}${file}" == *.zip ]]; then
    checkUnzip
    unzip -oq "${pathprefix}${file}" 2>&1 | printlines project="${project}" task="extract";
  else
    printError project="${project}" task="wget" msg="downloaded file not found. ${file}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && return || exit 1;
  fi
  if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    printError project="${project}" task="extract" msg="failed to extract ${file}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && return || exit 1;
  fi
}

############################################
#
# Function to clone a git repository or pull if it already exists locally
# Can optionally use a caching folder defined with SRC_CACHE_DIR
#
############################################
privatepullOrClone(){
  local project path dest tag recurse depth # reset first
  local "${@}"

  tgstr=""
  tgstr2=""
  if [ ! -z "${tag}" ] 
  then
    tgstr="origin tags/${tag}"
    tgstr2="-b ${tag}"
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
  
  should_clone=1
  if [ -d ${dest} ]; then
    unbufferCall "git -C ${dest} pull ${tgstr}" 2> /dev/null | printlines project="${project}" task="git"
    should_clone=${PIPESTATUS[0]}
    if [ $should_clone -ne 0 ]; then #Something failed. Remove to redownload
      rm -rf ${dest}
    fi
  fi

  if [ $should_clone -ne 0 ]; then
    unbufferCall "git clone -j$(nproc) $recursestr $depthstr $tgstr2 ${path} $dest" 2>&1 | printlines project="${project}" task="git"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="git" msg="failed to fetch ${path}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && return || exit 1;
    fi
  fi
}

pullOrClone (){
  local project path tag depth recurse ignorecache forcedownload noexit # reset first
  local "${@}"

  if [ -z "${forcedownload}" ] && [ $NO_DOWNLOAD -eq 1 ]; then
    printError project="${project}" task="git" msg="Download disabled. Missing dependency $path"
    [[ ! -z "${noexit}" ]] && FAILED=1 && return || exit 1
  fi

  printlines project="${project}" task="git" msg="clone ${tag}@${path}"
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
    privatepullOrClone project="${project}" dest=$dest_val tag=$tag depth=$depth recurse=$recurse path=$path
  elif [ -d "$dest_val" ] && [ ! -z "${tag}" ]; then #Folder exist, switch to tag
    currenttag=$(git -C $dest_val tag --points-at ${tag})
    printlines project="${project}" task="git" msg="TODO Check current tag \"${tag}\" == \"$currenttag\""
    privatepullOrClone project="${project}" dest=$dest_val tag=$tag depth=$depth recurse=$recurse path=$path
  elif [ -d "$dest_val" ] && [ -z "${tag}" ]; then #Folder exist, switch to main
    currenttag=$(git -C $dest_val tag --points-at ${tag})
    printlines project="${project}" task="git" msg="TODO Handle no tag \"${tag}\" == \"$currenttag\""
    privatepullOrClone project="${project}" dest=$dest_val tag=$tag depth=$depth recurse=$recurse path=$path
  elif [ ! -d "$dest_val" ]; then #fresh start
    privatepullOrClone project="${project}" dest=$dest_val tag=$tag depth=$depth recurse=$recurse path=$path
  else
    if [ -d "$dest_val" ]; then
      printlines project="${project}" task="git" msg="yey destval"
    fi
    if [ -z "${tag}" ]; then
      printlines project="${project}" task="git" msg="yey tag"
    fi
    printlines project="${project}" task="git" msg="1 $dest_val : $(test -f \"$dest_val\")"
    printlines project="${project}" task="git" msg="2 ${tag} : $(test -z \"${tag}\")"
  fi

  if [ $FAILED -eq 0 ] && [ ! -z "$SRC_CACHE_DIR" ] && [ -z "${ignorecache}" ]; then
    printlines project="${project}" task="git" msg="copy repo from cache"$"$dest_val"
    rm -rf "$name"
    cp -r "$dest_val" "./$name"
  fi
}

############################################
#
# Function to build a project configured with autotools
#
############################################
buildMakeProject(){
  local project srcdir prefix autogen autoreconf configure make cmakedir cmakeargs cmakeclean installargs skipbootstrap bootstrap configcustom outoftree noexit
  local "${@}"

  build_start=$SECONDS

  printlines project="${project}" task="build" msg="src: '${srcdir}'"
  printlines project="${project}" task="build" msg="prefix: '${prefix}'"
  
  if [ "${outoftree}" == "true" ] 
  then
    mkdir -p "${srcdir}/build"
    cd "${srcdir}/build"
    rel_path=".."
  else
    cd "${srcdir}"
    rel_path="."
  fi

  if [ -f "$rel_path/bootstrap" ] && [ -z "${skipbootstrap}" ]; then
    printlines project="${project}" task="bootstrap" msg="src: ${srcdir}"
    unbufferCall "$rel_path/bootstrap ${bootstrap}" 2>&1 | printlines project="${project}" task="bootstrap"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="bootstrap" msg="$rel_path/bootstrap failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi

  if [ -f "$rel_path/bootstrap.sh" ]; then
    printlines project="${project}" task="bootstrap.sh" msg="src: ${srcdir}"
    unbufferCall "$rel_path/bootstrap.sh ${bootstrap}" 2>&1 | printlines project="${project}" task="bootstrap.sh"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="bootstrap.sh" msg="$rel_path/bootstrap.sh failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi
  if [ -f "$rel_path/autogen.sh" ] && [ "${autogen}" != "skip" ]; then
    printlines project="${project}" task="autogen.sh" msg="src: ${srcdir}"
    unbufferCall "$rel_path/autogen.sh ${autogen}" 2>&1 | printlines project="${project}" task="autogen.sh"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="autogen.sh" msg="Autogen failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi

  if [ ! -z "${autoreconf}" ] 
  then
    printlines project="${project}" task="autoreconf" msg="src: ${srcdir}"
    currdir=$(pwd)
    cd "${rel_path}"
    unbufferCall "autoreconf ${autoreconf}" 2>&1 | printlines project="${project}" task="autoreconf"
    cd "${currdir}"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="autoreconf" msg="Autoreconf failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi
  
  if [ ! -z "${cmakedir}" ] 
  then
    printlines project="${project}" task="cmake" msg="src: ${srcdir}"
    printlines project="${project}" task="cmake" msg="arguments: ${cmakeargs}"
    if [ ! -z "${cmakeclean}" ]
    then 
      unbufferCall "cmake --build \"${cmakedir}\" --target clean" 2>&1 | printlines project="${project}" task="cmake"
      find . -iwholename '*cmake*' -not -name CMakeLists.txt -delete
    fi

    btype="Release"
    if [ $ENABLE_DEBUG -eq 1 ]; then
      btype="Debug"
    fi
    unbufferCall "cmake -G 'Unix Makefiles' " \
      "${cmakeargs} " \
      "-DCMAKE_BUILD_TYPE=$btype " \
      "-DCMAKE_INSTALL_PREFIX='${prefix}' " \
      "-DENABLE_TESTS=OFF " \
      "'${cmakedir}'" 2>&1 | printlines project="${project}" task="cmake"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="cmake" msg="failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi

  if [ ! -z "${configcustom}" ]; then
    printlines project="${project}" task="bash" msg="src: ${srcdir}"
    printlines project="${project}" task="bash" msg="command: ${configcustom}"
    unbufferCall "bash -c \"${configcustom}\"" 2>&1 | printlines project="${project}" task="bash"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="bash" msg="command failed: ${configcustom}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi

  if [ -f "$rel_path/configure" ] && [ -z "${cmakedir}" ]; then
    printlines project="${project}" task="configure" msg="src: ${srcdir}"
    printlines project="${project}" task="configure" msg="arguments: ${configure}"
    unbufferCall "$rel_path/configure " \
        "--prefix='${prefix}' " \
        "${configure}" 2>&1 | printlines project="${project}" task="configure"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="configure" msg="$rel_path/configure failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  else
    printlines project="${project}" task="configure" msg="no configuration available"
  fi

  printlines project="${project}" task="compile" msg="src: ${srcdir}"
  printlines project="${project}" task="compile" msg="arguments: '${makeargs}'"
  unbufferCall "make -j$(nproc) ${make}" 2>&1 | printlines project="${project}" task="compile"
  if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    printError project="${project}" task="compile" msg="make failed ${srcdir}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
  fi

  printlines project="${project}" task="install" msg="src: ${srcdir}"
  printlines project="${project}" task="install" msg="arguments: ${installargs}"
  unbufferCall "make -j$(nproc) ${make} install ${installargs}" 2>&1 | printlines project="${project}" task="install"
  if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    printError project="${project}" task="install" msg="failed ${srcdir}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
  fi

  build_time=$(( SECONDS - build_start ))
  displaytime project="${project}" task="build" time=$build_time label="Build runtime: "

  cd "$OLDPWD"
}

############################################
#
# Function to build a project configured with meson
#
############################################
buildMesonProject() {
  local project srcdir mesonargs prefix setuppatch bindir destdir builddir defaultlib clean noexit
  local "${@}"

  build_start=$SECONDS

  printlines project="${project}" task="meson" msg="src: '${srcdir}'"
  printlines project="${project}" task="meson" msg="prefix: '${prefix}'"

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
    rm -rf "${srcdir}/$build_dir"
  fi

  if [ ! -d "${srcdir}/$build_dir" ]; then
      mkdir -p "${srcdir}/$build_dir"

      cd "${srcdir}"
      if [ -d "./subprojects" ]; then
        printlines project="${project}" task="meson" msg="download subprojects ${srcdir}"
        #     meson subprojects download
      fi

      if [ ! -z "${setuppatch}" ]; then
        printlines project="${project}" task="bash" msg="src: ${srcdir}"
        printlines project="${project}" task="bash" msg="command: ${setuppatch}"
        unbufferCall "bash -c \"${setuppatch}\"" 2>&1 | printlines project="${project}" task="bash"
        if [ "${PIPESTATUS[0]}" -ne 0 ]; then
          printError project="${project}" task="bash" msg="command failed: ${srcdir}"
          [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
        fi
      fi
      btype="release"
      if [ $ENABLE_DEBUG -eq 1 ]; then
        btype="debug"
      fi
      printlines project="${project}" task="meson" msg="setup ${srcdir}"
      unbufferCall "meson setup $build_dir " \
          "${mesonargs} " \
          "--default-library=$default_lib " \
          "--prefix='${prefix}' " \
          "$bindir_val " \
          "--libdir=lib " \
          "--includedir=include " \
          "--buildtype=$btype" 2>&1 | printlines project="${project}" task="meson"
      if [ "${PIPESTATUS[0]}" -ne 0 ]; then
        printError project="${project}" task="meson" msg="setup failed ${srcdir}"
        rm -rf "$build_dir"
        [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
      fi
  else
      cd "${srcdir}"
      if [ -d "./subprojects" ]; then
        printlines project="${project}" task="meson" msg="update ${srcdir}"
        #     meson subprojects update
      fi

      if [ ! -z "${setuppatch}" ]; then
        printlines project="${project}" task="bash" msg="command: ${setuppatch}"
        unbufferCall "bash -c \"${setuppatch}\"" 2>&1 | printlines project="${project}" task="bash"
        if [ "${PIPESTATUS[0]}" -ne 0 ]; then
          printError project="${project}" task="bash" msg="command failed ${srcdir}"
          [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
        fi
      fi
      btype="release"
      if [ $ENABLE_DEBUG -eq 1 ]; then
        btype="debug"
      fi
      printlines project="${project}" task="meson" msg="reconfigure ${srcdir}"
      unbufferCall "meson setup $build_dir " \
          "${mesonargs} " \
          "--default-library=$default_lib " \
          "--prefix='${prefix}' " \
          "$bindir_val " \
          "--libdir=lib " \
          "--includedir=include " \
          "--buildtype=$btype " \
          "--reconfigure" 2>&1 | printlines project="${project}" task="meson"
      if [ "${PIPESTATUS[0]}" -ne 0 ]; then
        printError project="${project}" task="meson" msg="setup failed ${srcdir}"
        rm -rf "$build_dir"
        [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
      fi
  fi

  printlines project="${project}" task="meson" msg="compile ${srcdir}"
  printf '\033[?7l' #Small hack to prevent meson from cropping progress line
  unbufferCall "meson compile -C $build_dir" 2>&1 | printlines project="${project}" task="compile"
  status="${PIPESTATUS[0]}"
  printf '\033[?7h' #Small hack to prevent meson from cropping progress line
  if [ $status -ne 0 ]; then
    printError project="${project}" task="meson" msg="compile failed ${srcdir}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
  fi

  printlines project="${project}" task="meson" msg="install ${srcdir}"
  DESTDIR=${destdir} unbufferCall "meson install -C $build_dir" 2>&1 | printlines project="${project}" task="install"
  if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    printError project="${project}" task="meson" msg="install failed ${srcdir}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
  fi

  build_time=$(( SECONDS - build_start ))
  displaytime project="${project}" task="build" time=$build_time label="Build runtime: "
  cd "$OLDPWD"

}

pkgCheck() {
  local name minver project
  local "${@}"

  min_ver=""
  if [ ! -z ${minver} ]; then
    min_ver=" >= ${minver}"
  fi
  output=$(pkg-config --print-errors --errors-to-stdout "${name} $min_ver");
  if [ -z "$output" ]; then
    printlines project="${project}" task="check" msg="found"
  else
    printlines project="${project}" task="check" msg="not found"
    echo 1
  fi
}

progVersionCheck(){
  local major minor micro linenumber lineindex program paramoverride startcut length project
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
      printlines project="${project}" task="check" msg="not found"
      echo 1
      return
  fi

  FIRSTLINE=$(${program} ${varparam} | head -${linenumber})
  firstLineArray=(${FIRSTLINE//;/ })
  versionString=${firstLineArray[${lineindex}]};

  [[ ! -z "${startcut}" ]] && [[ -z "${length}" ]] && versionString=${versionString:2}
  [[ ! -z "${startcut}" ]] && [[ ! -z "${length}" ]] && versionString=${versionString:$startcut:$length}
  [[ -z "${startcut}" ]] && [[ ! -z "${length}" ]] && versionString=${versionString:0:$length}

  versionArray=(${versionString//./ })
  actualmajor=${versionArray[0]}
  actualminor=${versionArray[1]}
  actualmicro=${versionArray[2]}

  if [ -z "$major"  ]; then 
    printlines project="${project}" task="check" msg="found"
    return; 
  fi

  if [[ -z "$actualmajor"  || ! "$actualmajor" == ?(-)+([[:digit:]]) ]]; then echo "Unexpected ${program} version ouput[1] \"$FIRSTLINE\" \"$versionString\""; return; fi
  if [ "$actualmajor" -gt "$major" ]; then
    printlines project="${project}" task="check" msg="found"
    return;
  elif [ "$actualmajor" -lt "$major" ]; then
    printlines project="${project}" task="check" msg="version is too old. $actualmajor.$actualminor.$actualmicro < $major.$minor.$micro"
    echo 1
  else
    if [ -z "$minor"  ]; then return; fi
    if [[ -z "$actualminor"  || ! "$actualminor" == ?(-)+([[:digit:]]) ]]; then echo "Unexpected ${program} version ouput[2] \"$FIRSTLINE\" \"$versionString\""; return; fi
    if [ "$actualminor" -gt "$minor" ]; then
      printlines project="${project}" task="check" msg="found"
      return;
    elif [ "$actualminor" -lt "$minor" ]; then
      printlines project="${project}" task="check" msg="version is too old. $actualmajor.$actualminor.$actualmicro < $major.$minor.$micro"
      echo 1
    else
      if [ -z "$micro"  ]; then return; fi
      if [[ -z "$actualmicro"  || ! "$actualmicro" == ?(-)+([[:digit:]]) ]]; then echo "Unexpected ${program} version ouput[3] \"$FIRSTLINE\" \"$versionString\""; return; fi
      if [ ! -z "$micro" ] && [ "$actualmicro" -lt "$micro" ]; then
        printlines project="${project}" task="check" msg="version is too old. $actualmajor.$actualminor.$actualmicro < $major.$minor.$micro"
        echo 1
      else
        printlines project="${project}" task="check" msg="found"
        return
      fi
    fi
  fi
}

function checkLibrary {
  local name static project
  local "${@}"

  [[ ! -z "${static}" ]] && staticstr=" -static" || staticstr=""

  output="$(gcc -l${name}${staticstr} 2>&1)";
  if [[ "$output" == *"undefined reference to \`main'"* ]]; then
    printlines project="${project}" task="check" msg="found"
  else
    printlines project="${project}" task="check" msg="not found"
    echo 1
  fi
}

export PATH="$SUBPROJECT_DIR/unzip60/build/dist/bin":$PATH
HAS_UNZIP=0
checkUnzip(){
  if [ $HAS_UNZIP -eq 0 ] && [ ! -z "$(progVersionCheck project="unzip" program=unzip paramoverride="-v")" ]; then
    downloadAndExtract project="unzip" file="unzip60.tar.gz" path="https://sourceforge.net/projects/infozip/files/UnZip%206.x%20%28latest%29/UnZip%206.0/unzip60.tar.gz/download"
    buildMakeProject project="unzip" srcdir="unzip60" make="-f unix/Makefile generic" installargs=" prefix=$SUBPROJECT_DIR/unzip60/build/dist MANDIR=$SUBPROJECT_DIR/unzip60/build/dist/share/man/man1 -f unix/Makefile"
  fi
  HAS_UNZIP=1
}

export PATH="$SUBPROJECT_DIR/gettext-0.21.1/dist/bin":$PATH
HAS_GETTEXT=0
checkGetTextAndAutoPoint(){
  if [ $HAS_GETTEXT -eq 0 ] && \
     ([ ! -z "$(progVersionCheck project="gettext" program=gettextize linenumber=1 lineindex=3 major=0 minor=9 micro=18 )" ] || 
      [ ! -z "$(progVersionCheck project="autopoint" program=autopoint linenumber=1 lineindex=3 major=0 minor=9 micro=18 )" ]); then
    downloadAndExtract project="gettext" file="gettext-0.21.1.tar.gz" path="https://ftp.gnu.org/pub/gnu/gettext/gettext-0.21.1.tar.gz"
    buildMakeProject project="gettext" srcdir="gettext-0.21.1" prefix="$SUBPROJECT_DIR/gettext-0.21.1/dist" autogen="skip"
  fi
  HAS_GETTEXT=1
}


export PERL5LIB="$SUBPROJECT_DIR/perl-5.40.0/lib":$PERL5LIB
export PATH="$SUBPROJECT_DIR/perl-5.40.0/build/bin":$PATH
HAS_PERL=0
checkPerl(){
  if [ $HAS_PERL -eq 0 ] && \
      [ ! -z "$(progVersionCheck project="perl" program=perl linenumber=2 lineindex=8 major=5 minor=38 micro=0 startcut=2 length=6)" ]; then
      downloadAndExtract project="perl" file="perl-5.40.0.tar.gz" path="https://www.cpan.org/src/5.0/perl-5.40.0.tar.gz"
      buildMakeProject project="perl" srcdir="perl-5.40.0" skipbootstrap="true" configcustom="./Configure -des -Dprefix=$SUBPROJECT_DIR/perl-5.40.0/build"
  fi
  HAS_PERL=1
}


export PATH="$SUBPROJECT_DIR/glibc-2.40/build/dist/bin":$PATH
if [ ! -z "$LIBRARY_PATH" ]; then
  export LIBRARY_PATH="$SUBPROJECT_DIR/glibc-2.40/build/dist/lib":$LIBRARY_PATH
else
  export LIBRARY_PATH="$SUBPROJECT_DIR/glibc-2.40/build/dist/lib"
fi

HAS_GLIBC=0
checkGlibc(){
  if [ $HAS_GLIBC -eq 0 ] && \
      [ ! -z "$(checkLibrary project="glibc" name=c static=true)" ]; then
      downloadAndExtract project="glibc" file="glibc-2.40.tar.xz" path="https://ftp.gnu.org/gnu/glibc/glibc-2.40.tar.xz"
      buildMakeProject project="glibc" srcdir="glibc-2.40" prefix="$SUBPROJECT_DIR/glibc-2.40/build/dist" skipbootstrap="true" outoftree="true"
  fi
  HAS_GLIBC=1
}

checkGitRepoState(){
  local repo package
  local "${@}"
  
  ret=0
  git -C ${repo} remote update &> /dev/null
  LOCAL=$(git -C ${repo} rev-parse @ 2> /dev/null)
  REMOTE=$(git -C ${repo} rev-parse @{u} 2> /dev/null)
  BASE=$(git -C ${repo} merge-base @ @{u} 2> /dev/null)

  if [ ! -z "$LOCAL" ] && [ $LOCAL = $REMOTE ]; then
    printlines project="${package}" task="git" msg="${repo} is already up-to-date. Do nothing..."
  elif [ ! -z "$LOCAL" ] && [ $LOCAL = $BASE ]; then
    printlines project="${package}" task="git" msg="${repo} has new changes. Force rebuild..."
    ret=1
  elif [ ! -z "$LOCAL" ] && [ $REMOTE = $BASE ]; then
    printlines project="${package}" task="git" msg="${repo} has local changes. Doing nothing..."
  elif [ ! -z "$LOCAL" ]; then
    printlines project="${package}" task="git" msg="Error ${repo} is diverged."
    ret=-1
  else
    printlines project="${package}" task="git" msg="not found"
    ret=-1
  fi

  if [ $ret == 0 ] && [ ! -z "${package}" ] && [ ! -z "$(pkgCheck name=${package} project=${project})" ]; then
    ret=1
  fi

  echo $ret
}

VENV_SETUP=0
setupPythonVirtualEnv(){

  if [ $VENV_SETUP -eq 1 ]; then
    return
  fi

  VENV_EXEC=""
  if [ ! -z "$(progVersionCheck project="virtualenv" program=virtualenv)" ]; then
    if [ $NO_DOWNLOAD -eq 0 ] && [ ! -f "virtualenv.pyz" ]; then
      $WGET_CMD https://bootstrap.pypa.io/virtualenv.pyz 2>&1 | printlines project="virtualenv" task="wget"
      if [ "${PIPESTATUS[0]}" -ne 0 ]; then
        printError project="virtualenv" task="wget" msg="failed to fetch https://bootstrap.pypa.io/virtualenv.pyz"
        exit 1;
      fi
    fi
    VENV_EXEC="python3 virtualenv.pyz"
  else
    VENV_EXEC="virtualenv"
  fi

  # Setting up Virtual Python environment
  if [ ! -f "./venvfolder/bin/activate" ] && [ ! -f "./venvfolder/local/bin/activate" ]; then
    $VENV_EXEC ./venvfolder 2>&1 | printlines project="virtualenv" task="setup"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="virtualenv" task="setup" msg="failed to run python3 virtualenv.pyz"
      exit 1;
    fi
  else
    printlines project="virtualenv" task="check" msg="found"
  fi

  # Activate virtual environment
  ACTIVATE=""

  if [ -f "./venvfolder/bin/activate" ]; then
    ACTIVATE="./venvfolder/bin/activate"
  elif [ -f "./venvfolder/local/bin/activate" ]; then
    ACTIVATE="./venvfolder/local/bin/activate"
  fi

  if [ -z "$ACTIVATE" ]; then
    printError project="virtualenv" task="activate" msg="failed to activate python virtual environment."
    exit 1;
  else
    source "$ACTIVATE"
    printlines project="virtualenv" task="activate" msg="activated python virtual environment."
  fi

  #Setup meson
  if [ $NO_DOWNLOAD -eq 0 ]; then
    if [ ! -z "$(progVersionCheck project="meson" program=meson linenumber=1 lineindex=0 major=1 minor=1)" ]; then
      unbufferCall "python3 -m pip install --progress-bar on meson --upgrade" 2>&1 | printlines project="meson" task="pip"
      if [ "${PIPESTATUS[0]}" -ne 0 ]; then
        printError project="meson" task="install" msg="Failed to install meson via pip. Please try to install meson manually."
        exit 1
      fi
    fi

    if [ ! -z "$(progVersionCheck project="ninja" program=ninja)" ]; then
      unbufferCall "python3 -m pip install --progress-bar on ninja --upgrade" 2>&1 | printlines project="ninja" task="pip"
      if [ "${PIPESTATUS[0]}" -ne 0 ]; then
        printError project="ninja" task="check" msg="Failed to install ninja via pip. Please try to install ninja manually."
        exit 1
      fi
    fi
  else
    printlines project="onvifmgr" task="check" msg="Skipping pip install. NO_DOWNLOAD is set."
  fi
  VENV_SETUP=1
}

if [ $GEN_ICONS -eq 1 ]; then
  generateIcons
  exit 0
fi

# Hard dependency check
MISSING_DEP=0
if [ ! -z "$(progVersionCheck project="make" program='make')" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="g++" program=g++)" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="git" program=git)" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="tar" program=tar)" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="wget" program=wget)" ]; then
  MISSING_DEP=1
elif [ -z "$(progVersionCheck project="wget" program=wget linenumber=1 lineindex=2 major=2)" ]; then
  WGET_CMD="wget --force-progress" #Since version 2+ show-progress ins't supported
elif [ -z "$(progVersionCheck project="wget" program=wget linenumber=1 lineindex=2 major=1 minor=16)" ]; then
  WGET_CMD="wget --show-progress --progress=bar:force" #Since version 1.16 set show-progress
fi

if [ $MISSING_DEP -eq 1 ]; then
  printError project="onvifmgr" task="check" msg="Missing core dependency program"
  exit 1
fi

unbuffer echo "test" 2> /dev/null 1> /dev/null
[ $? == 0 ] && UNBUFFER_COMMAND="unbuffer" && printlines project="unbuffer" task="check" msg="found" || printlines project="unbuffer" task="check" msg="not found"

mkdir -p "$SUBPROJECT_DIR"
mkdir -p "$SRC_CACHE_DIR"

cd "$SUBPROJECT_DIR"

#Setup pkg-config before package checking starts
if [ ! -z "$(progVersionCheck project="pkg-config" program=pkg-config)" ]; then
  export PATH=:"$SUBPROJECT_DIR/pkgconf/build/dist/bin":$PATH
  export PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH
  if [ ! -z "$(progVersionCheck project="pkg-config" program=pkgconf)" ]; then
    setupPythonVirtualEnv
    pullOrClone project="pkgconf" path=https://github.com/pkgconf/pkgconf.git tag=pkgconf-2.4.3
    buildMesonProject project="pkgconf" srcdir="pkgconf" prefix="$SUBPROJECT_DIR/pkgconf/build/dist" mesonargs=""
    ln $SUBPROJECT_DIR/pkgconf/build/dist/bin/pkgconf $SUBPROJECT_DIR/pkgconf/build/dist/bin/pkg-config
  fi
fi

if [ ! -z "$(pkgCheck project="gtk3" name=gtk+-3.0)" ]; then
  MISSING_DEP=1
fi

if [ $MISSING_DEP -eq 1 ]; then
  printError project="onvifmgr" task="check" msg="Missing core dependency package"
  exit 1
fi

export ACLOCAL_PATH="$ACLOCAL_PATH:/usr/share/aclocal" #When using locally built autoconf, we need to add system path
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:"$(pkg-config --variable pc_path pkg-config)" #When using locally built pkgconf, we need to add system path

#Setup gnu tools
# export PATH="$SUBPROJECT_DIR/make-4.4.1/build/dist/bin":$PATH
# if [ ! -z "$(progVersionCheck program=make linenumber=1 lineindex=2 major=4 minor=4 micro=1 )" ]; then
#   echo "building make from source..."
#   downloadAndExtract file="make-4.4.1.tar.gz" path="https://ftp.gnu.org/gnu/make/make-4.4.1.tar.gz"
#   if [ $FAILED -eq 1 ]; then exit 1; fi
#   buildMakeProject srcdir="make-4.4.1" prefix="$SUBPROJECT_DIR/make-4.4.1/build/dist" skipbootstrap="true"
#   if [ $FAILED -eq 1 ]; then exit 1; fi
# else
#   echo "make already installed."
# fi

export PATH="$SUBPROJECT_DIR/m4-1.4.20/build/dist/bin":$PATH
if [ ! -z "$(progVersionCheck project="m4" program=m4 linenumber=1 lineindex=3 major=1 minor=4 micro=18 )" ]; then
  downloadAndExtract project="m4" file="m4-1.4.20.tar.xz" path="https://ftp.gnu.org/gnu/m4/m4-1.4.20.tar.xz"
  buildMakeProject project="m4" srcdir="m4-1.4.20" prefix="$SUBPROJECT_DIR/m4-1.4.20/build/dist" skipbootstrap="true"
fi

export PATH="$SUBPROJECT_DIR/autoconf-2.72/build/dist/bin":$PATH
export ACLOCAL_PATH="$SUBPROJECT_DIR/autoconf-2.72/build/dist/share/autoconf":$ACLOCAL_PATH
if [ ! -z "$(progVersionCheck project="autoconf" program=autoconf linenumber=1 lineindex=3 major=2 minor=70)" ]; then
  checkPerl
  downloadAndExtract project="autoconf" file="autoconf-2.72.tar.xz" path="https://ftp.gnu.org/gnu/autoconf/autoconf-2.72.tar.xz"
  buildMakeProject project="autoconf" srcdir="autoconf-2.72" prefix="$SUBPROJECT_DIR/autoconf-2.72/build/dist" skipbootstrap="true"
fi

export PATH="$SUBPROJECT_DIR/automake-1.17/build/dist/bin":$PATH
export ACLOCAL_PATH="$SUBPROJECT_DIR/automake-1.17/build/dist/share/aclocal-1.17":$ACLOCAL_PATH
if [ ! -z "$(progVersionCheck project="automake" program=automake linenumber=1 lineindex=3 major=1 minor=16 micro=1)" ]; then
  downloadAndExtract project="automake" file="automake-1.17.tar.xz" path="https://ftp.gnu.org/gnu/automake/automake-1.17.tar.xz"
  buildMakeProject project="automake" srcdir="automake-1.17" prefix="$SUBPROJECT_DIR/automake-1.17/build/dist" skipbootstrap="true"
fi

export PATH="$SUBPROJECT_DIR/libtool-2.5.3/build/dist/bin":$PATH
export ACLOCAL_PATH="$SUBPROJECT_DIR/libtool-2.5.3/build/dist/share/aclocal":$ACLOCAL_PATH
if [ ! -z "$(progVersionCheck project="libtool" program=libtoolize linenumber=1 lineindex=3 major=2 minor=4 micro=6)" ]; then
  downloadAndExtract project="libtool" file="libtool-2.5.3.tar.xz" path="https://ftp.gnu.org/gnu/libtool/libtool-2.5.3.tar.xz"
  buildMakeProject project="libtool" srcdir="libtool-2.5.3" prefix="$SUBPROJECT_DIR/libtool-2.5.3/build/dist" skipbootstrap="true"
fi

export PATH="$SUBPROJECT_DIR/flex-2.6.4/build/dist/bin":$PATH
if [ ! -z "$(progVersionCheck project="flex" program=flex linenumber=1 lineindex=1 major=2 minor=6 micro=4 )" ]; then
  checkGetTextAndAutoPoint
  downloadAndExtract project="flex" file="flex-2.6.4.tar.gz" path="https://github.com/westes/flex/releases/download/v2.6.4/flex-2.6.4.tar.gz"
  buildMakeProject project="flex" srcdir="flex-2.6.4" prefix="$SUBPROJECT_DIR/flex-2.6.4/build/dist" skipbootstrap="true"
fi

export PATH="$SUBPROJECT_DIR/bison-3.8.2/build/dist/bin":$PATH
if [ ! -z "$(progVersionCheck project="bison" program=bison linenumber=1 lineindex=3 major=3 minor=5 micro=1 )" ]; then
  downloadAndExtract project="bison" file="bison-3.8.2.tar.xz" path="https://ftp.gnu.org/gnu/bison/bison-3.8.2.tar.xz"
  buildMakeProject project="bison" srcdir="bison-3.8.2" prefix="$SUBPROJECT_DIR/bison-3.8.2/build/dist" skipbootstrap="true"
fi

#TODO only call this if necessary
setupPythonVirtualEnv

# export PKG_CONFIG_PATH="$SUBPROJECT_DIR/gtk/dist/lib/pkgconfig":$PKG_CONFIG_PATH
# echo $SUBPROJECT_DIR/gtk/dist/lib/pkgconfig
# pkgCheck project="gtk" name=gtk+-3.0 minver=3.24.43
# if [ ! -z "$(pkgCheck project="gtk" name=gtk+-3.0 minver=3.24.43)" ]; then
#   #https://download.gnome.org/sources/gtk+/3.24/gtk%2B-3.24.42.tar.xz
#   pullOrClone project="gtk" path="https://gitlab.gnome.org/GNOME/gtk.git" tag="3.24.43"
#   buildMesonProject project="gtk" srcdir="gtk" prefix="$SUBPROJECT_DIR/gtk/dist" mesonargs="-Dtests=false -Dintrospection=false -Ddemos=false -Dexamples=false -Dlibepoxy:tests=false"
# fi

################################################################
# 
#    Build Openssl for gsoap and onvifsoap
#       
################################################################
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/openssl/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
if [ ! -z "$(pkgCheck project="openssl" name=libcrypto minver=1.1.1f)" ]; then
  checkGlibc
  pullOrClone project="openssl" path="https://github.com/openssl/openssl.git" tag="OpenSSL_1_1_1w"
  buildMakeProject project="openssl" srcdir="openssl" configcustom="./config --prefix='$SUBPROJECT_DIR/openssl/build/dist' --openssldir='$SUBPROJECT_DIR/openssl/build/dist/ssl' -static"
fi

export PATH="$SUBPROJECT_DIR/cmake-4.0.2/build/dist/bin":$PATH
if [ ! -z "$(progVersionCheck project="cmake" program=cmake linenumber=1 lineindex=2 major=4 minor=0 micro=0 )" ]; then
  downloadAndExtract project="cmake" file="cmake-4.0.2.tar.gz" path="https://github.com/Kitware/CMake/releases/download/v4.0.2/cmake-4.0.2.tar.gz"
  buildMakeProject project="cmake" srcdir="cmake-4.0.2" prefix="$SUBPROJECT_DIR/cmake-4.0.2/build/dist" skipbootstrap="true" configure="--parallel=$(nproc) --no-debugger"
fi

################################################################
# 
#     Build glib dependency
#   sudo apt-get install libglib2.0-dev (gstreamer minimum 2.64.0)
# 
################################################################
export PATH="$SUBPROJECT_DIR/glib-2.85.2/dist/bin":$PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/glib-2.85.2/dist/lib/pkgconfig":$PKG_CONFIG_PATH
if [ ! -z "$(pkgCheck project="glib" name=glib-2.0 minver=2.64.0)" ] || [ ! -z "$(progVersionCheck project="glib-mkenums" program=glib-mkenums)" ]; then
  downloadAndExtract project="glib" file="glib-2.85.2.tar.xz" path="https://download.gnome.org/sources/glib/2.85/glib-2.85.2.tar.xz"
  buildMesonProject project="glib" srcdir="glib-2.85.2" prefix="$SUBPROJECT_DIR/glib-2.85.2/dist" mesonargs="-Dsysprof=disabled -Dpcre2:test=false -Dpcre2:grep=false -Dxattr=false -Db_lundef=false -Dtests=false -Dglib_debug=disabled -Dglib_assert=false -Dglib_checks=false"
fi

################################################################
# 
#    Build cutils
#       
################################################################
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/CUtils/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
rebuild=$(checkGitRepoState project="cutils" repo="CUtils" package="cutils")

if [ $rebuild != 0 ]; then
  #OnvifSoapLib depends on it and will also require a rebuild
  rm -rf "$SUBPROJECT_DIR/CUtils/build"
  rm -rf "$SUBPROJECT_DIR/OnvifSoapLib/build"
  
  pullOrClone project="cutils" path=https://github.com/Quedale/CUtils.git ignorecache="true"
  #Clean up previous build in case of failure. This will prevent from falling back on the old version
  rm -rf "$SUBPROJECT_DIR/CUtils/build/dist/*"
  buildMakeProject project="cutils" srcdir="CUtils" prefix="$SUBPROJECT_DIR/CUtils/build/dist" cmakedir=".." outoftree=true cmakeclean=true
fi

export PKG_CONFIG_PATH="$SUBPROJECT_DIR/libntlm-1.8/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
if [ ! -z "$(pkgCheck project="libntlm" name=libntlm minver=v1.5)" ]; then
  downloadAndExtract project="libntlm" file="libntlm-1.8.tar.gz" path="https://download-mirror.savannah.gnu.org/releases/libntlm/libntlm-1.8.tar.gz"
  buildMakeProject project="libntlm" srcdir="libntlm-1.8" skipbootstrap="true" prefix="$SUBPROJECT_DIR/libntlm-1.8/build/dist" configure="--enable-shared=no" #TODO support shared linking
fi

################################################################
# 
#    Build OnvifSoapLib
#       
################################################################
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/OnvifSoapLib/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
export PATH="$SUBPROJECT_DIR/gsoap-2.8/build/dist/bin":$PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/gsoap-2.8/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/zlib-1.2.13/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
rebuild=$(checkGitRepoState repo="OnvifSoapLib" project="onvifsoap" package="onvifsoap")

if [ $rebuild != 0 ]; then
  #Clean up previous build in case of failure. This will prevent from falling back on the old version
  rm -rf "$SUBPROJECT_DIR/OnvifSoapLib/build/dist/*" 

  if [ ! -z "$(pkgCheck project="zlib" name=zlib minver=1.2.11)" ]; then
    downloadAndExtract project="zlib" file="zlib-1.2.13.tar.gz" path="https://www.zlib.net/zlib-1.2.13.tar.gz"
    buildMakeProject project="zlib" srcdir="zlib-1.2.13" prefix="$SUBPROJECT_DIR/zlib-1.2.13/build/dist"
  fi

  gsoap_version=2.8.134
  if [ ! -z "$(pkgCheck project="gsoap" name=gsoap minver=$gsoap_version)" ]; then
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

    rm -rf "$SUBPROJECT_DIR/gsoap-2.8/" #Make sure we dont extract over an old version
    downloadAndExtract project="gsoap" file="gsoap_$gsoap_version.zip" path="https://sourceforge.net/projects/gsoap2/files/gsoap_$gsoap_version.zip/download"
    C_INCLUDE_PATH="$inc_path" \
    CPLUS_INCLUDE_PATH="$inc_path" \
    LIBRARY_PATH="$SSL_LIBS:$ZLIB_LIBS:$LIBRARY_PATH" \
    LD_LIBRARY_PATH="$SSL_LIBS:$ZLIB_LIBS:$LD_LIBRARY_PATH" \
    LIBS='-ldl -lpthread' \
    buildMakeProject project="gsoap" srcdir="gsoap-2.8" prefix="$SUBPROJECT_DIR/gsoap-2.8/build/dist" autogen="skip" configure="--with-openssl=/usr/lib/ssl"
  fi

  pullOrClone project="onvifsoap" path=https://github.com/Quedale/OnvifSoapLib.git ignorecache="true"

  nodownload=""
  if [ "$NO_DOWNLOAD" -eq 1 ]; then nodownload="--no-download"; fi

  GSOAP_SRC_DIR="$SUBPROJECT_DIR/gsoap-2.8" \
  C_INCLUDE_PATH="$(pkg-config --variable=includedir openssl):$(pkg-config --variable=includedir zlib):$C_INCLUDE_PATH" \
  buildMakeProject project="onvifsoap" srcdir="OnvifSoapLib" prefix="$SUBPROJECT_DIR/OnvifSoapLib/build/dist" cmakedir=".." cmakeargs="-DGSOAP_SRC_DIR='$SUBPROJECT_DIR/gsoap-2.8'" bootstrap="$nodownload --skip-gsoap $skipwsdl" outoftree=true cmakeclean=true
fi

################################################################
# 
#    Build alsa-lib dependency
#   sudo apt install llibasound2-dev (tested 1.2.7.2)
# 
################################################################
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/alsa-lib/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
if [ ! -z "$(pkgCheck project="alsa" name=alsa minver=1.2.2)" ]; then
  pullOrClone project="alsa" path=git://git.alsa-project.org/alsa-lib.git tag=v1.2.8
  buildMakeProject project="alsa" srcdir="alsa-lib" prefix="$SUBPROJECT_DIR/alsa-lib/build/dist" configure="--enable-static=no --enable-shared=yes" autoreconf="-vif"
fi

################################################################
# 
#    Build wayland dependencies
#       This is a soft dependency to support GstGlDisplay under wayland
#       
################################################################
export PATH="$SUBPROJECT_DIR/wayland/dist/bin":$PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/wayland/dist/lib/pkgconfig":$PKG_CONFIG_PATH
if [ ! -z "$(pkgCheck project="wayland-client" name=wayland-client minver=1.24.0)" ] ||
    [ ! -z "$(pkgCheck project="wayland-cursor" name=wayland-cursor minver=1.24.0)" ] ||
    [ ! -z "$(pkgCheck project="wayland-scanner" name=wayland-scanner minver=1.24.0)" ] ||
    [ ! -z "$(pkgCheck project="wayland-egl" name=wayland-egl minver=1.24.0)" ]; then
  pullOrClone project="wayland" path=https://gitlab.freedesktop.org/wayland/wayland.git tag=1.24.0
  buildMesonProject project="wayland" srcdir="wayland" prefix="$SUBPROJECT_DIR/wayland/dist" mesonargs="-Dtests=false -Ddocumentation=false"
fi

export PKG_CONFIG_PATH="$SUBPROJECT_DIR/wayland-protocols/dist/share/pkgconfig":$PKG_CONFIG_PATH
if [ ! -z "$(pkgCheck project="wayland-protocols" name=wayland-protocols minver=1.45)" ]; then
  pullOrClone project="wayland-protocols" path=https://gitlab.freedesktop.org/wayland/wayland-protocols.git tag=1.45
  buildMesonProject project="wayland-protocols" srcdir="wayland-protocols" prefix="$SUBPROJECT_DIR/wayland-protocols/dist" mesonargs=""
fi

################################################################
# 
#    Build Gstreamer dependency
#       sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
################################################################
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/FFmpeg/dist/lib/pkgconfig":$PKG_CONFIG_PATH

gst_ret=0
GSTREAMER_LATEST=1.27.2
if [ $ENABLE_LATEST == 0 ]; then
  GSTREAMER_VERSION=1.14.4
else
  GSTREAMER_VERSION=$GSTREAMER_LATEST
fi

gst_core=(
  "gstreamer-1.0;gstreamer-1.0"
  "gstreamer-plugins-base-1.0;gstreamer-plugins-base-1.0"
  "gstreamer-plugins-good-1.0;" #Dont check for this package since it doesnt exists in arch
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
    "opus;gstopus"
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
    # "kms;gstkms"
    # "wayland;gstwaylandsink" #This is mandatory otherwise gstgtkwayland will be skipped
    # "gtk3;gstgtkwayland"
    "interlace;gstinterlace"
    "openh264;" #Openh264 is an optional package
    "fdkaac;gstfdkaac"
    "videoparsers;gstvideoparsersbad"
    "onvif;gstrtponvif"
    "jpegformat;gstjpegformat"
    "v4l2codecs;gstv4l2codecs"
    "libde265;" #libde265 is an optional package
)
if [ $ENABLE_NVCODEC != 0 ]; then gst_bad_plugins+=("nvcodec;gstnvcodec"); fi

checkGstreamerPkg () {
  local static version project # reset first
  local "${@}"

  for gst_p in ${gst_core[@]}; do
    IFS=";" read -r -a arr <<< "${gst_p}"
    if [ ! -z "${arr[1]}" ] && [ ! -z "$(pkgCheck project=${arr[0]} name=${arr[1]} minver=${version})" ]; then
      printf "  missing core package ${arr[0]} >= ${version}\n";
    fi
  done

  for gst_p in ${gst_base_plugins[@]}; do
    IFS=";" read -r -a arr <<< "${gst_p}"
    if [ ! -z "${arr[1]}" ] && [ ! -z "$(checkLibrary project=${project}-${arr[0]} name=${arr[1]} static=${static})" ]; then
      printf "  missing base plugin ${arr[0]} >= ${version}\n";
    fi
  done
  for gst_p in ${gst_good_plugins[@]}; do
    IFS=";" read -r -a arr <<< "${gst_p}"
    if [ ! -z "${arr[1]}" ] && [ ! -z "$(checkLibrary project=${project}-${arr[0]} name=${arr[1]} static=${static})" ]; then
      printf "  missing good plugin ${arr[0]} >= ${version}\n";
    fi
  done
  for gst_p in ${gst_bad_plugins[@]}; do
    IFS=";" read -r -a arr <<< "${gst_p}"
    if [ ! -z "${arr[1]}" ] && [ ! -z "$(checkLibrary project=${project}-${arr[0]} name=${arr[1]} static=${static})" ]; then
      printf "  missing bad plugin ${arr[0]} >= ${version}\n";
    fi
  done
}

og_lib_path=$LIBRARY_PATH
export LIBRARY_PATH=/usr/lib/gstreamer-1.0:/usr/local/lib/gstreamer-1.0:$LIBRARY_PATH
#Gstreamer install on system doesn't break down by plugin, but groups them under base,good,bad,ugly
if [ ! -z "$(checkGstreamerPkg project="gstreamer" version=$GSTREAMER_VERSION)" ]; then
  gst_ret=1;
fi
export LIBRARY_PATH=$og_lib_path

export PKG_CONFIG_PATH="$SUBPROJECT_DIR/pulseaudio/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/libde265/dist/lib/pkgconfig":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/libX11-1.8.10/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/macros/dist/lib/pkgconfig":$PKG_CONFIG_PATH

export PKG_CONFIG_PATH="$SUBPROJECT_DIR/systemd-256/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/libgudev/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/libcap/dist/lib64/pkgconfig":"$SUBPROJECT_DIR/libcap/dist/pkgconfig":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/util-linux/dist/lib/pkgconfig":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/libsndfile/dist/lib/pkgconfig":"$SUBPROJECT_DIR/libsndfile/dist/lib64/pkgconfig":$PKG_CONFIG_PATH
export PATH="$SUBPROJECT_DIR/gperf-3.1/dist/bin":$PATH
export PATH="$SUBPROJECT_DIR/nasm-2.16.03/dist/bin":$PATH
export PATH="$SUBPROJECT_DIR/FFmpeg/dist/bin":$PATH
export ACLOCAL_PATH="$SUBPROJECT_DIR/macros/dist/lib/aclocal":$ACLOCAL_PATH
export ACLOCAL_PATH="$SUBPROJECT_DIR/libxtrans/dist/share/aclocal":$ACLOCAL_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/libxtrans/dist/share/pkgconfig":$PKG_CONFIG_PATH

#Check to see if gstreamer exist on the system
if [ $gst_ret != 0 ] || [ $ENABLE_LATEST != 0 ]; then
  export PKG_CONFIG_PATH="$SUBPROJECT_DIR/gstreamer/build_omx/dist/lib/gstreamer-1.0/pkgconfig":$PKG_CONFIG_PATH
  export PKG_CONFIG_PATH="$SUBPROJECT_DIR/gstreamer/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
  export PKG_CONFIG_PATH="$SUBPROJECT_DIR/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig":$PKG_CONFIG_PATH

  gst_ret=0;
  GSTREAMER_VERSION=$GSTREAMER_LATEST; #If we are to build something, build latest
  #Gstreamer static plugins needs to be checked individually
  og_lib_path=$LIBRARY_PATH
  export LIBRARY_PATH=$SUBPROJECT_DIR/gstreamer/build/dist/lib/gstreamer-1.0:$SUBPROJECT_DIR/gstreamer/build/dist/lib:$LIBRARY_PATH
  if [ ! -z "$(checkGstreamerPkg project="gstreamer-src" version=$GSTREAMER_VERSION static=true)" ]; then
    gst_ret=1;
  fi
  if [ $ENABLE_LIBAV -eq 1 ] && [ ! -z "$(pkgCheck project="gstreamer-libav" name=gstlibav minver=$GSTREAMER_VERSION)" ]; then
    gst_ret=1;
  fi
  export LIBRARY_PATH=$og_lib_path

  #Global check if gstreamer is already built
  if [ $gst_ret != 0 ]; then
    checkGetTextAndAutoPoint

    ################################################################
    # 
    #    Build gudev-1.0 dependency
    #   sudo apt install libgudev-1.0-dev (tested 232)
    # 
    ################################################################
    if [ ! -z "$(pkgCheck project="gudev" name=gudev-1.0 minver=232)" ]; then
      if [ ! -z "$(progVersionCheck project="gperf" program=gperf)" ]; then
        downloadAndExtract project="gperf" file="gperf-3.1.tar.gz" path="http://ftp.gnu.org/pub/gnu/gperf/gperf-3.1.tar.gz"
        buildMakeProject project="gperf" srcdir="gperf-3.1" prefix="$SUBPROJECT_DIR/gperf-3.1/dist" autogen="skip"
      fi

      if [ ! -z "$(pkgCheck project="libcap" name=libcap minver=2.32)" ]; then
        #old link git://git.kernel.org/pub/scm/linux/kernel/git/morgan/libcap.git
        pullOrClone project="libcap" path=https://kernel.googlesource.com/pub/scm/libs/libcap/libcap tag=libcap-2.69
        buildMakeProject project="libcap" srcdir="libcap" prefix="$SUBPROJECT_DIR/libcap/dist" installargs="DESTDIR='$SUBPROJECT_DIR/libcap/dist'"
      fi

      #TODO build autopoint... (Mint linux)
      if [ ! -z "$(pkgCheck project="mount" name=mount minver=2.34.0)" ]; then
        pullOrClone project="mount" path=https://github.com/util-linux/util-linux.git tag=v2.38.1
        buildMakeProject project="mount" srcdir="util-linux" prefix="$SUBPROJECT_DIR/util-linux/dist" configure="--disable-rpath --disable-bash-completion --disable-makeinstall-setuid --disable-makeinstall-chown"
      fi

      python3 -c 'from setuptools import setup'
      if [ $? != 0 ]; then
        printlines project="setuptools" task="check" msg="not found"
        unbufferCall "python3 -m pip install --progress-bar on setuptools --upgrade" 2>&1 | printlines project="setuptools" task="pip"
        if [ "${PIPESTATUS[0]}" -ne 0 ]; then
          printError project="setuptools" task="install" msg="Failed to setuptools ninja via pip. Please try to install setuptools manually."
          exit 1
        fi
      else
        printlines project="setuptools" task="check" msg="found"
      fi

      python3 -c 'import jinja2' 2> /dev/null
      ret=$?
      if [ $ret != 0 ]; then
        printlines project="jinja2" task="jinja2" msg="not found"
        unbufferCall "python3 -m pip install --progress-bar on jinja2 --upgrade" 2>&1 | printlines project="jinja2" task="pip"
        if [ "${PIPESTATUS[0]}" -ne 0 ]; then
          printError project="jinja2" task="install" msg="Failed to jinja2 ninja via pip. Please try to install jinja2 manually."
          exit 1
        fi
      else
        printlines project="jinja2" task="check" msg="found"
      fi

      if [ ! -z "$(pkgCheck project="udev" name=libudev minver=256)" ]; then
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
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsysvinit-path='$SUBPROJECT_DIR/systemd-256/build/dist/init.d'"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbashcompletiondir='$SUBPROJECT_DIR/systemd-256/build/dist/bash-completion'"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcreate-log-dirs=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlocalstatedir='$SUBPROJECT_DIR/systemd-256/build/dist/localstate'"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsshconfdir='$SUBPROJECT_DIR/systemd-256/build/dist/sshcfg'"
        downloadAndExtract project="udev" file="v256.tar.gz" path="https://github.com/systemd/systemd/archive/refs/tags/v256.tar.gz"
        C_INCLUDE_PATH="$SUBPROJECT_DIR/libcap/dist/usr/include" \
        LIBRARY_PATH="$SUBPROJECT_DIR/libcap/dist/lib64":"$SUBPROJECT_DIR/libcap/dist":$LIBRARY_PATH \
        buildMesonProject project="udev" srcdir="systemd-256" prefix="$SUBPROJECT_DIR/systemd-256/build/dist" mesonargs="$SYSD_MESON_ARGS"
      fi

      pullOrClone project="gudev" path=https://gitlab.gnome.org/GNOME/libgudev.git tag=237
      C_INCLUDE_PATH="$SUBPROJECT_DIR/systemd-256/build/dist/include" \
      LIBRARY_PATH="$SUBPROJECT_DIR/libcap/dist/lib64":"$SUBPROJECT_DIR/systemd-256/build/dist/lib" \
      buildMesonProject project="gudev" srcdir="libgudev" prefix="$SUBPROJECT_DIR/libgudev/build/dist" mesonargs="-Dvapi=disabled -Dtests=disabled -Dintrospection=disabled"
    fi

    ################################################################
    # 
    #    Build libpulse dependency
    #   sudo apt install libpulse-dev (tested 12.2)
    # 
    ################################################################
    if [ ! -z "$(pkgCheck project="pulse" name=libpulse minver=12.2)" ]; then

      if [ ! -z "$(pkgCheck project="sndfile" name=sndfile minver=1.2.0)" ]; then
        LIBSNDFILE_CMAKEARGS="-DBUILD_EXAMPLES=off"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DBUILD_TESTING=off"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DBUILD_SHARED_LIBS=on"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DINSTALL_PKGCONFIG_MODULE=on"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DINSTALL_MANPAGES=off"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DCMAKE_POLICY_VERSION_MINIMUM=3.5" #Necessary to build with cmake version 4.x
        pullOrClone project="sndfile" path="https://github.com/libsndfile/libsndfile.git" tag=1.2.2
        mkdir "libsndfile/build"
        buildMakeProject project="sndfile" srcdir="libsndfile/build" prefix="$SUBPROJECT_DIR/libsndfile/dist" cmakedir=".." cmakeargs="$LIBSNDFILE_CMAKEARGS"
      fi

      PULSE_MESON_ARGS="-Ddaemon=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Ddoxygen=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dman=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dtests=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Ddatabase=simple"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dbashcompletiondir=no"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dzshcompletiondir=no"
      pullOrClone project="pulse" path="https://gitlab.freedesktop.org/pulseaudio/pulseaudio.git" tag=v16.1
      buildMesonProject project="pulse" srcdir="pulseaudio" prefix="$SUBPROJECT_DIR/pulseaudio/build/dist" mesonargs="$PULSE_MESON_ARGS"
    fi

    ################################################################
    # 
    #    Build nasm dependency
    #   sudo apt install nasm
    # 
    ################################################################
    if [ ! -z "$(progVersionCheck project="nasm" program=nasm)" ]; then
      downloadAndExtract project="nasm" file=nasm-2.16.03.tar.gz path=https://www.nasm.us/pub/nasm/releasebuilds/2.16.03/nasm-2.16.03.tar.gz
      buildMakeProject project="nasm" srcdir="nasm-2.16.03" prefix="$SUBPROJECT_DIR/nasm-2.16.03/dist"
    fi

    if [ ! -z "$(pkgCheck project="libde265" name=libde265 minver=v1.0.15)" ]; then
      pullOrClone project="libde265" path="https://github.com/strukturag/libde265.git" tag=v1.0.15
      buildMakeProject project="libde265" srcdir="libde265" prefix="$SUBPROJECT_DIR/libde265/dist" configure="--disable-sherlock265"
    fi

    if [ ! -z "$(pkgCheck project="x11-xcb" name=x11-xcb minver=1.7.2)" ]; then
      if [ ! -z "$(pkgCheck project="xmacro" name=xorg-macros minver=1.19.1)" ]; then
        pullOrClone project="xmacro" path="https://gitlab.freedesktop.org/xorg/util/macros.git" tag="util-macros-1.20.0"
        buildMakeProject project="xmacro" srcdir="macros" prefix="$SUBPROJECT_DIR/macros/dist" configure="--datarootdir='$SUBPROJECT_DIR/macros/dist/lib'"
      fi

      if [ ! -z "$(pkgCheck project="xtrans" name=xtrans minver=1.4.0)" ]; then
        pullOrClone project="xtrans" path="https://gitlab.freedesktop.org/xorg/lib/libxtrans.git" tag="xtrans-1.5.1"
        buildMakeProject project="xtrans" srcdir="libxtrans" prefix="$SUBPROJECT_DIR/libxtrans/dist"
      fi

      downloadAndExtract project="x11" file="libX11-1.8.10.tar.xz" path="https://www.x.org/archive/individual/lib/libX11-1.8.10.tar.xz"
      buildMakeProject project="x11" srcdir="libX11-1.8.10" prefix="$SUBPROJECT_DIR/libX11-1.8.10/build/dist" outoftree="true"
    fi

    MESON_PARAMS=""
    if [ $ENABLE_LIBAV -eq 1 ]; then
        
        [[ ! -z "$(pkgCheck project="ffmpeg-libavcodec" name=libavcodec minver=58.20.100)" ]] && ret1=1 || ret1=0
        [[ ! -z "$(pkgCheck project="ffmpeg-libavfilter" name=libavfilter minver=7.40.101)" ]] && ret2=1 || ret2=0
        [[ ! -z "$(pkgCheck project="ffmpeg-libavformat" name=libavformat minver=58.20.100)" ]] && ret3=1 || ret3=0
        [[ ! -z "$(pkgCheck project="ffmpeg-libavutil" name=libavutil minver=56.22.100)" ]] && ret4=1 || ret4=0
        [[ ! -z "$(pkgCheck project="ffmpeg-libpostproc" name=libpostproc minver=55.3.100)" ]] && ret5=1 || ret5=0
        [[ ! -z "$(pkgCheck project="ffmpeg-libswresample" name=libswresample minver=3.3.100)" ]] && ret6=1 || ret6=0
        [[ ! -z "$(pkgCheck project="ffmpeg-libswscale" name=libswscale minver=5.3.100)" ]] && ret7=1 || ret7=0

        if [ $ret1 != 0 ] || [ $ret2 != 0 ] || [ $ret3 != 0 ] || [ $ret4 != 0 ] || [ $ret5 != 0 ] || [ $ret6 != 0 ] || [ $ret7 != 0 ]; then
            #######################
            #
            # Custom FFmpeg build
            #   For some reason, Gstreamer's meson dep doesn't build any codecs
            #   
            #   sudo apt install libavcodec-dev libavfilter-dev libavformat-dev libswresample-dev
            #######################
            FFMPEG_CONFIGURE_ARGS="--disable-lzma"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --disable-doc"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --disable-shared"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-static"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-nonfree"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-version3"
            FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-gpl"

            pullOrClone project="ffmpeg" path="https://github.com/FFmpeg/FFmpeg.git" tag=n7.1.1
            buildMakeProject project="ffmpeg" srcdir="FFmpeg" prefix="$SUBPROJECT_DIR/FFmpeg/dist" configure="$FFMPEG_CONFIGURE_ARGS"
            rm -rf "$SUBPROJECT_DIR/FFmpeg/dist/lib/*.so"
        else
            printlines project="ffmpeg" task="check" msg="found"
        fi
        MESON_PARAMS="$MESON_PARAMS -Dlibav=enabled"
    fi

    pullOrClone project="gstreamer" path="https://gitlab.freedesktop.org/gstreamer/gstreamer.git" tag=$GSTREAMER_VERSION
    
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
    CFLAGS='-DGST_DISABLE_GST_DEBUG=1 -DGST_DISABLE_REGISTRY=1' \
    LIBRARY_PATH=$LIBRARY_PATH:"$SUBPROJECT_DIR/systemd-256/dist/lib" \
    buildMesonProject project="gstreamer" srcdir="gstreamer" prefix="$SUBPROJECT_DIR/gstreamer/build/dist" mesonargs="$MESON_PARAMS" builddir="build"
  else
      printlines project="latest-gstreamer" task="check" msg="found"
  fi
else
    printlines project="gstreamer" task="check" msg="found"
fi

generateIcons

printlines project="onvifmgr" task="build" msg="bootstrap"
#Change to the project folder to run autoconf and automake
cd "$SCRT_DIR"

# Clean up any previous autotools cache to ensure a clean bootstrap
rm -rf autom4te.cache aclocal.m4 configure

#Initialize project with correct autotools sequence
aclocal -I m4 2>&1 | printlines project="onvifmgr" task="aclocal"
[ "${PIPESTATUS[0]}" -ne 0 ] && printError project="onvifmgr" task="aclocal" msg="Failed to bootstrap OnvifDeviceManager" && exit 1
libtoolize --force --copy 2>&1 | printlines project="onvifmgr" task="libtoolize"
[ "${PIPESTATUS[0]}" -ne 0 ] && printError project="onvifmgr" task="libtoolize" msg="Failed to bootstrap OnvifDeviceManager" && exit 1
autoconf 2>&1 | printlines project="onvifmgr" task="autoconf"
[ "${PIPESTATUS[0]}" -ne 0 ] && printError project="onvifmgr" task="autoconf" msg="Failed to bootstrap OnvifDeviceManager" && exit 1
automake --add-missing --copy 2>&1 | printlines project="onvifmgr" task="automake"
[ "${PIPESTATUS[0]}" -ne 0 ] && printError project="onvifmgr" task="automake" msg="Failed to bootstrap OnvifDeviceManager" && exit 1
#Running autoreconf otherwise calling make will reconfigure the project
autoreconf 2>&1 | printlines project="onvifmgr" task="autoreconf"
[ "${PIPESTATUS[0]}" -ne 0 ] && printError project="onvifmgr" task="autoreconf" msg="Failed to bootstrap OnvifDeviceManager" && exit 1

configArgs=$@
printlines project="onvifmgr" task="build" msg="configure $configArgs"
cd "$WORK_DIR"

PKG_CONFIG_PATH=$og_pkg_path $SCRT_DIR/configure $configArgs 2>&1 | printlines project="onvifmgr" task="configure"
if [ "${PIPESTATUS[0]}" -ne 0 ]; then
  printError project="onvifmgr" task="build" msg="Failed to configure OnvifDeviceManager"
else
  displaytime project="onvifmgr" task="build" time=$(( SECONDS - script_start )) msg="OnvifDeviceManager is ready to be built."$'\n'$'\t'"Simply run \"make -j\$(nproc)\"" label="Script runtime: "
fi
