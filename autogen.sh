#!/bin/bash
SKIP=0
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

ENABLE_LATEST=0
ENABLE_LIBAV=0
ENABLE_NVCODEC=0

i=1;

for arg in "$@" 
do
    shift
    if [ "$arg" == "--enable-libav=yes" ] || [ "$arg" == "--enable-libav=true" ]; then
        ENABLE_LIBAV=1
        set -- "$@" "$arg"
    elif [ "$arg" == "--enable-latest" ]; then
        ENABLE_LATEST=1
        set -- "$@" "$arg"
    elif [ "$arg" == "--enable-nvcodec=yes" ] || [ "$arg" == "--enable-nvcodec=true" ]; then
        ENABLE_NVCODEC=1
        set -- "$@" "$arg"
    else
        set -- "$@" "$arg"
    fi
    i=$((i + 1));
done


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

############################################
#
# Function to download and extract tar package file
# Can optionally use a caching folder defined with SRC_CACHE_DIR
#
############################################
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
  elif [[ $dest_val == *.zip ]]; then
    unzip -o $dest_val
  else
    echo "ERROR FILE NOT FOUND ${path} // ${file} // $dest_val"
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
  local path tag depth recurse ignorecache # reset first
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
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** Copy repo from cache ***\n${NC}"
    printf "${ORANGE}*** $dest_val ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
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
  local srcdir prefix autogen autoreconf configure make cmakedir cmakeargs installargs bootstrap configcustom outoftree
  local "${@}"

  build_start=$SECONDS
  if [ $SKIP -eq 1 ]; then
      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Skipping Make ${srcdir} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      return
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}* Building Project ***\n${NC}"
  printf "${ORANGE}* Src dir : ${srcdir} ***\n${NC}"
  printf "${ORANGE}* Prefix : ${prefix} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"

  curr_dir=$(pwd)
  cd ${srcdir}

  rel_path="."
  if [ "${outoftree}" == "true" ] 
  then
    mkdir -p build
    cd build
    rel_path=".."
  fi

  if [ -f "$rel_path/bootstrap" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** bootstrap ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    $rel_path/bootstrap ${bootstrap}
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** $rel_path/bootstrap failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        FAILED=1
        cd $curr_dir
        return
    fi
  fi

  if [ -f "$rel_path/bootstrap.sh" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** bootstrap.sh ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    $rel_path/bootstrap.sh ${bootstrap}
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** $rel_path/bootstrap.sh failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        FAILED=1
        cd $curr_dir
        return
    fi
  fi
  if [ -f "$rel_path/autogen.sh" ] && [ "${autogen}" != "skip" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** autogen ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    $rel_path/autogen.sh ${autogen}
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** Autogen failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        FAILED=1
        cd $curr_dir
        return
    fi
  fi

  if [ ! -z "${autoreconf}" ] 
  then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** autoreconf ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    autoreconf ${autoreconf}
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** Autoreconf failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        FAILED=1
        cd $curr_dir
        return
    fi
  fi
  if [ ! -z "${cmakedir}" ] 
  then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** cmake ${srcdir} ***\n${NC}"
    printf "${ORANGE}*** Args ${cmakeargs} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    cmake -G "Unix Makefiles" \
      ${cmakeargs} \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="${prefix}" \
      -DENABLE_TESTS=OFF \
      -DENABLE_SHARED=on \
      "${cmakedir}"
    status=$?
    if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** CMake failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
      FAILED=1
      cd $curr_dir
      return
    fi
  fi

  if [ ! -z "${configcustom}" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** custom config ${srcdir} ***\n${NC}"
    printf "${ORANGE}*** ${configcustom} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
      bash -c "${configcustom}"
    status=$?
    if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** Custom Config failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
      FAILED=1
      cd $curr_dir
      return
    fi
  fi

  if [ -f "$rel_path/configure" ] && [ -z "${cmakedir}" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** configure ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    $rel_path/configure \
        --prefix=${prefix} \
        ${configure}
    status=$?
    if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** $rel_path/configure failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
      FAILED=1
      cd $curr_dir
      return
    fi
  else
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** no configuration available ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** compile ${srcdir} ***\n${NC}"
  printf "${ORANGE}*** Make Args : ${makeargs} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  make -j$(nproc) ${make}
  status=$?
  if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** Make failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
      FAILED=1
      cd $curr_dir
      return
  fi


  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** install ${srcdir} ***\n${NC}"
  printf "${ORANGE}*** Make Args : ${makeargs} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  make -j$(nproc) ${make} install ${installargs}
  status=$?
  if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** Make failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
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
  if [ $SKIP -eq 1 ]
  then
      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Skipping Meson ${srcdir} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      return
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}* Building Github Project ***\n${NC}"
  printf "${ORANGE}* Src dir : ${srcdir} ***\n${NC}"
  printf "${ORANGE}* Prefix : ${prefix} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"

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
          printf "${ORANGE}*****************************\n${NC}"
          printf "${ORANGE}*** Download Subprojects ${srcdir} ***\n${NC}"
          printf "${ORANGE}*****************************\n${NC}"
          #     $MESON_TOOL subprojects download
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
              FAILED=1
              cd $curr_dir
              return
          fi
      fi

      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Meson Setup ${srcdir} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      $MESON_TOOL setup $build_dir \
          ${mesonargs} \
          --default-library=$default_lib \
          --prefix=${prefix} \
          $bindir_val \
          --libdir=lib \
          --includedir=include \
          --buildtype=release 
      status=$?
      if [ $status -ne 0 ]; then
          printf "${RED}*****************************\n${NC}"
          printf "${RED}*** Meson Setup failed ${srcdir} ***\n${NC}"
          printf "${RED}*****************************\n${NC}"
          rm -rf $build_dir
          FAILED=1
          cd $curr_dir
          return
      fi
  else
      cd ${srcdir}
      if [ -d "./subprojects" ]; then
          printf "${ORANGE}*****************************\n${NC}"
          printf "${ORANGE}*** Meson Update ${srcdir} ***\n${NC}"
          printf "${ORANGE}*****************************\n${NC}"
          #     $MESON_TOOL subprojects update
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
              FAILED=1
              cd $curr_dir
              return
          fi
      fi

      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Meson Reconfigure $(pwd) ${srcdir} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      $MESON_TOOL setup $build_dir \
          ${mesonargs} \
          --default-library=$default_lib \
          --prefix=${prefix} \
          $bindir_val \
          --libdir=lib \
          --includedir=include \
          --buildtype=release \
          --reconfigure

      status=$?
      if [ $status -ne 0 ]; then
          printf "${RED}*****************************\n${NC}"
          printf "${RED}*** Meson Setup failed ${srcdir} ***\n${NC}"
          printf "${RED}*****************************\n${NC}"
          rm -rf $build_dir
          FAILED=1
          cd $curr_dir
          return
      fi
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** Meson Compile ${srcdir} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  $MESON_TOOL compile -C $build_dir
  status=$?
  if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** meson compile failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
      FAILED=1
      cd $curr_dir
      return
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** Meson Install ${srcdir} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  DESTDIR=${destdir} $MESON_TOOL install -C $build_dir
  status=$?
  if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** Meson Install failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
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
# Function to check if a program exists
#
############################################
checkProg () {
  local name args path # reset first
  local "${@}"

  if !PATH=$PATH:${path} command -v ${name} &> /dev/null
  then
    return #Prog not found
  else
    PATH=$PATH:${path} command ${name} ${args} &> /dev/null
    status=$?
    if [[ $status -eq 0 ]]; then
        echo "Working"
    else
        return #Prog failed
    fi
  fi
}

isMesonInstalled(){
  local major minor micro # reset first
  local "${@}"

  ret=0
  MESONVERSION=$(meson --version)
  ret=$?
  if [[ $status -eq 0 ]]; then
    IFS='.' read -ra ADDR <<< "$MESONVERSION"
    index=0
    success=0
    for i in "${ADDR[@]}"; do
      if [ "$index" == "0" ]; then
        valuetocomapre="${major}" #Major version
      elif [ "$index" == "1" ]; then
        valuetocomapre="${minor}" #Minor version
      elif [ "$index" == "2" ]; then
        valuetocomapre="${micro}" #Micro version
      fi
      
      if [ "$i" -gt "$valuetocomapre" ]; then
        success=1
        break
      elif [ "$i" -lt "$valuetocomapre" ]; then
        success=0
        break
      fi

      if [ "$index" == "2" ]; then
        success=1
      fi
      ((index=index+1))
    done

    if [ $success -eq "1" ]; then
      return
    else
      echo "nope"
    fi

  else
    echo "nope"
  fi
}

# Hard dependency check
MISSING_DEP=0
if [ -z "$(checkProg name='make' args='--version' path=$PATH)" ]; then
 MISSING_DEP=1
 echo "make build utility not found! Aborting..."
fi


if [ -z "$(checkProg name='bison' args='--version' path=$PATH)" ]; then
  MISSING_DEP=1
  echo "bison build utility not found! Aborting..."
fi

if [ -z "$(checkProg name='flex' args='--version' path=$PATH)" ]; then
  MISSING_DEP=1
  echo "flex build utility not found! Aborting..."
fi

if [ -z "$(checkProg name='automake' args='--version' path=$PATH)" ]; then
  MISSING_DEP=1
  echo "automake build utility not found! Aborting..."
fi

if [ -z "$(checkProg name='autoconf' args='--version' path=$PATH)" ]; then
  MISSING_DEP=1
  echo "autoconf build utility not found! Aborting..."
fi

if [ -z "$(checkProg name='libtoolize' args='--version' path=$PATH)" ]; then
  MISSING_DEP=1
  echo "libtool build utility not found! Aborting..."
fi

if [ -z "$(checkProg name='pkg-config' args='--version' path=$PATH)" ]; then
  MISSING_DEP=1
  echo "pkg-config build utility not found! Aborting..."
fi

if [ -z "$(checkProg name='g++' args='--version' path=$PATH)" ]; then
 MISSING_DEP=1
 echo "g++ build utility not found! Aborting..."
fi

pkg-config --exists "gtk+-3.0"
ret=$?
if [ $ret != 0 ]; then
  MISSING_DEP=1
  echo "libgtk-3-dev build utility not found! Aborting..."
fi


if [ $MISSING_DEP -eq 1 ]; then
  exit 1
fi

#Change to the project folder to run autoconf and automake
cd $SCRT_DIR

#Initialize project
aclocal
autoconf
automake --add-missing
autoreconf -i

mkdir -p $SUBPROJECT_DIR
mkdir -p $SRC_CACHE_DIR

cd $SUBPROJECT_DIR

#Download virtualenv tool
if [ ! -f "virtualenv.pyz" ]; then
  wget https://bootstrap.pypa.io/virtualenv.pyz
fi

#Setting up Virtual Python environment
if [ ! -f "./venvfolder/bin/activate" ]; then
  python3 virtualenv.pyz ./venvfolder
fi

#Activate virtual environment
source venvfolder/bin/activate

#Setup meson
if [ ! -z "$(isMesonInstalled major=0 minor=63 micro=2)" ]; then
  echo "ninja build utility not found. Installing from pip..."
  python3 -m pip install meson --upgrade
else
  echo "Meson $($MESON_TOOL --version) already installed."
fi

if [ -z "$(checkProg name='ninja' args='--version' path=$PATH)" ]; then
  echo "ninja build utility not found. Installing from pip..."
  python3 -m pip install ninja --upgrade
else
  echo "Ninja $(ninja --version) already installed."
fi

# pullOrClone path="https://gitlab.gnome.org/GNOME/gtk.git" tag="3.24.34"
# buildMesonProject srcdir="gtk" prefix="$SUBPROJECT_DIR/gtk/dist" mesonargs="-Dtests=false -Dintrospection=false -Ddemos=false -Dexamples=false -Dlibepoxy:tests=false"
# if [ $FAILED -eq 1 ]; then exit 1; fi

################################################################
# 
#    Build unzip for gsoap
#       
################################################################
PATH=$SUBPROJECT_DIR/unzip60/build/dist/bin:$PATH
if [ -z "$(checkProg name='unzip' args='-v' path=$PATH)" ]; then
  # pullOrClone path="https://git.launchpad.net/ubuntu/+source/unzip" tag="applied/6.0-28"
  downloadAndExtract file="unzip60.tar.gz" path="https://sourceforge.net/projects/infozip/files/UnZip%206.x%20%28latest%29/UnZip%206.0/unzip60.tar.gz/download"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  buildMakeProject srcdir="unzip60" make="-f unix/Makefile generic" installargs="prefix=$SUBPROJECT_DIR/unzip60/build/dist MANDIR=$SUBPROJECT_DIR/unzip60/build/dist/share/man/man1 -f unix/Makefile"
  if [ $FAILED -eq 1 ]; then exit 1; fi
fi

################################################################
# 
#    Build Openssl for gsoap, onvifdisco and onvifsoap
#       
################################################################
OPENSSL_PKG=$SUBPROJECT_DIR/openssl/build/dist/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$OPENSSL_PKG \
pkg-config --exists --print-errors "libcrypto >= 1.1.1f"
ret=$?
if [ $ret != 0 ]; then 
  pullOrClone path="https://github.com/openssl/openssl.git" tag="OpenSSL_1_1_1q"
  buildMakeProject srcdir="openssl" configcustom="./config --prefix=$SUBPROJECT_DIR/openssl/build/dist --openssldir=$SUBPROJECT_DIR/openssl/build/dist/ssl -static"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "openssl already found."
fi

ZLIB_PKG=$SUBPROJECT_DIR/zlib-1.2.13/build/dist/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$ZLIB_PKG \
pkg-config --exists --print-errors "zlib >= 1.2.11"
ret=$?
if [ $ret != 0 ]; then 
  downloadAndExtract file="zlib-1.2.13.tar.gz" path="https://www.zlib.net/zlib-1.2.13.tar.gz"
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
GSOAP_PKG=$SUBPROJECT_DIR/gsoap-2.8/build/dist/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GSOAP_PKG \
pkg-config --exists --print-errors "gsoap >= 2.8.123"
ret=$?
if [ $ret != 0 ]; then 

  SSL_PREFIX=$(PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$OPENSSL_PKG pkg-config --variable=prefix openssl)
  if [ "$SSL_PREFIX" != "/usr" ]; then
    SSL_INCLUDE=$(PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$OPENSSL_PKG pkg-config --variable=includedir openssl)
    SSL_LIBS=$(PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$OPENSSL_PKG pkg-config --variable=libdir openssl)
  fi
  
  ZLIB_PREFIX=$(PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$ZLIB_PKG pkg-config --variable=prefix zlib)
  if [ "$ZLIB_PREFIX" != "/usr" ]; then
    ZLIB_INCLUDE=$(PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$ZLIB_PKG pkg-config --variable=includedir zlib)
    ZLIB_LIBS=$(PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$ZLIB_PKG pkg-config --variable=libdir zlib)
  fi

  echo "-- Building gsoap libgsoap-dev --"
  downloadAndExtract file="gsoap.zip" path="https://sourceforge.net/projects/gsoap2/files/gsoap_2.8.129.zip/download"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$OPENSSL_PKG:$ZLIB_PKG \
  C_INCLUDE_PATH="$SSL_INCLUDE:$ZLIB_INCLUDE:$C_INCLUDE_PATH" \
  CPLUS_INCLUDE_PATH="$SSL_INCLUDE:$ZLIB_INCLUDE:$CPLUS_INCLUDE_PATH" \
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
PKG_GLIB=$SUBPROJECT_DIR/glib-2.74.1/dist/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_GLIB \
pkg-config --exists --print-errors "glib-2.0 >= 2.64.0"
ret=$?
if [ $ret != 0 ]; then 
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
#    Build OnvifDiscoveryLib
#       
################################################################
#Check if new changes needs to be pulled
git -C OnvifDiscoveryLib remote update 2> /dev/null
LOCAL=$(git -C OnvifDiscoveryLib rev-parse @)
REMOTE=$(git -C OnvifDiscoveryLib rev-parse @{u})
BASE=$(git -C OnvifDiscoveryLib merge-base @ @{u})
force_rebuild=0
if [ $LOCAL = $REMOTE ]; then
    echo "OnvifDiscoveryLib is already up-to-date. Do nothing..."
elif [ $LOCAL = $BASE ]; then
    echo "OnvifDiscoveryLib has new changes. Force rebuild..."
    force_rebuild=1
    skipwsdl="--skip-wsdl"
elif [ $REMOTE = $BASE ]; then
    echo "OnvifDiscoveryLib has local changes. Doing nothing..."
    skipwsdl="--skip-wsdl"
else
    echo "Error OnvifDiscoveryLib is diverged."
    exit 1
fi

if [ $force_rebuild -eq 0 ]; then
  DISCOLIB_PKG=$SUBPROJECT_DIR/OnvifDiscoveryLib/build/dist/lib/pkgconfig
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$DISCOLIB_PKG \
  pkg-config --exists --print-errors "onvifdisco"
  ret=$?
else
  ret=1
fi

if [ $ret != 0 ]; then
  echo "-- Bootrap OnvifDiscoveryLib  --"
  pullOrClone path=https://github.com/Quedale/OnvifDiscoveryLib.git ignorecache="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  PATH=$SUBPROJECT_DIR/gsoap-2.8/build/dist/bin:$PATH \
  GSOAP_SRC_DIR=$SUBPROJECT_DIR/gsoap-2.8 \
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$OPENSSL_PKG \
  C_INCLUDE_PATH="$(pkg-config --variable=includedir openssl):$(pkg-config --variable=includedir zlib):$C_INCLUDE_PATH" \
  buildMakeProject srcdir="OnvifDiscoveryLib" prefix="$SUBPROJECT_DIR/OnvifDiscoveryLib/build/dist" bootstrap="--skip-gsoap $skipwsdl" outoftree=true
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "onvifdisco already found."
fi

################################################################
# 
#    Build OnvifSoapLib
#       
################################################################
#Check if new changes needs to be pulled
git -C OnvifSoapLib remote update 2> /dev/null
LOCAL=$(git -C OnvifSoapLib rev-parse @)
REMOTE=$(git -C OnvifSoapLib rev-parse @{u})
BASE=$(git -C OnvifSoapLib merge-base @ @{u})
force_rebuild=0
if [ $LOCAL = $REMOTE ]; then
    echo "OnvifSoapLib is already up-to-date. Do nothing..."
elif [ $LOCAL = $BASE ]; then
    echo "OnvifSoapLib has new changes. Force rebuild..."
    force_rebuild=1
    skipwsdl="--skip-wsdl"
elif [ $REMOTE = $BASE ]; then
    echo "OnvifSoapLib has local changes. Doing nothing..."
    skipwsdl="--skip-wsdl"
else
    echo "Error OnvifSoapLib is diverged."
    exit 1
fi

#Git is up to date, now check if built
if [ $force_rebuild -eq 0 ]; then
  ONVIFSOAP_PKG=$SUBPROJECT_DIR/OnvifSoapLib/build/dist/lib/pkgconfig
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$ONVIFSOAP_PKG \
  pkg-config --exists --print-errors "onvifsoap"
  ret=$?
else
  ret=1
fi

if [ $ret != 0 ]; then
  echo "-- Bootstrap OnvifSoapLib  --"
  pullOrClone path=https://github.com/Quedale/OnvifSoapLib.git ignorecache="true"
  if [ $FAILED -eq 1 ]; then exit 1; fi
  PATH=$SUBPROJECT_DIR/gsoap-2.8/build/dist/bin:$PATH \
  GSOAP_SRC_DIR=$SUBPROJECT_DIR/gsoap-2.8 \
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$OPENSSL_PKG \
  C_INCLUDE_PATH="$(pkg-config --variable=includedir openssl):$(pkg-config --variable=includedir zlib):$C_INCLUDE_PATH" \
  buildMakeProject srcdir="OnvifSoapLib" prefix="$SUBPROJECT_DIR/OnvifSoapLib/build/dist" cmakedir=".." bootstrap="--skip-gsoap $skipwsdl" outoftree=true
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
ALSA_PKG=$SUBPROJECT_DIR/alsa-lib/build/dist/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$ALSA_PKG \
pkg-config --exists --print-errors "alsa >= 1.2.6.1"
ret=$?
if [ $ret != 0 ]; then
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
FFMPEG_PKG=$SUBPROJECT_DIR/FFmpeg/dist/lib/pkgconfig
GST_OMX_PKG_PATH=$SUBPROJECT_DIR/gstreamer/build_omx/dist/lib/gstreamer-1.0/pkgconfig
GST_PKG_PATH=:$SUBPROJECT_DIR/gstreamer/build/dist/lib/pkgconfig:$SUBPROJECT_DIR/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig
gst_ret=0
gst_nv_ret=0
gst_libav_ret=0
gst_plg_base=0
gst_plg_good=0
gst_plg_bad=0
GSTREAMER_LATEST=1.22.5

pkg-config --exists --print-errors "gstreamer-1.0 >= 1.14.4"
gst_ret=$?
pkg-config --exists --print-errors "gstreamer-plugins-base-1.0 >= 1.14.4"
gst_plg_base=$?
pkg-config --exists --print-errors "gstreamer-plugins-good-1.0 >= 1.14.4"
gst_plg_good=$?
pkg-config --exists --print-errors "gstreamer-plugins-bad-1.0 >= 1.14.4"
gst_plg_bad=$?
if [ $ENABLE_LIBAV -eq 1 ]; then
  pkg-config --exists --print-errors "gstlibav >= 1.14.4"
  gst_libav_ret=$?
fi

if [ $ENABLE_NVCODEC -eq 1 ]; then
  pkg-config --exists --print-errors "gstnvcodec >= 1.14.4"
  gst_nv_ret=$?
fi

#Check to see if gstreamer exist on the system
if [ $gst_ret != 0 ] || [ $gst_libav_ret != 0 ] || [ $gst_nv_ret != 0 ] || [ $ENABLE_LATEST != 0 ] || [ $gst_plg_base != 0 ] || [ $gst_plg_good != 0 ] || [ $gst_plg_bad != 0 ]; then

  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_PKG_PATH:$PKG_GLIB \
  pkg-config --exists --print-errors "gstreamer-1.0 >= $GSTREAMER_LATEST"
  gst_ret=$?
  if [ $ENABLE_LIBAV -eq 1 ]; then
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_PKG_PATH:$PKG_GLIB:$FFMPEG_PKG \
    pkg-config --exists --print-errors "gstlibav >= $GSTREAMER_LATEST"
    gst_libav_ret=$?
  fi

  if [ $ENABLE_NVCODEC -eq 1 ]; then
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_PKG_PATH:$PKG_GLIB \
    pkg-config --exists --print-errors "gstnvcodec >= $GSTREAMER_LATEST"
    gst_nv_ret=$?
  fi

  #Global check if gstreamer is already built
  if [ $gst_ret != 0 ] || [ $gst_libav_ret != 0 ] || [ $gst_nv_ret != 0 ]; then
    ################################################################
    # 
    #    Build gettext and libgettext dependency
    #   sudo apt install gettext libgettext-dev (tested 1.16.3)
    # 
    ################################################################
    PATH=$PATH:$SUBPROJECT_DIR/gettext-0.21.1/dist/bin
    if [ -z "$(checkProg name='gettextize' args='--version' path=$PATH)" ] || [ -z "$(checkProg name='autopoint' args='--version' path=$PATH)" ]; then
      echo "not found gettext"
      downloadAndExtract file="gettext-0.21.1.tar.gz" path="https://ftp.gnu.org/pub/gnu/gettext/gettext-0.21.1.tar.gz"
      if [ $FAILED -eq 1 ]; then exit 1; fi
      buildMakeProject srcdir="gettext-0.21.1" prefix="$SUBPROJECT_DIR/gettext-0.21.1/dist" autogen="skip"
      if [ $FAILED -eq 1 ]; then exit 1; fi
    else
      echo "gettext already found."
    fi

    ################################################################
    # 
    #    Build gudev-1.0 dependency
    #   sudo apt install libgudev-1.0-dev (tested 232)
    # 
    ################################################################
    PKG_UDEV=$SUBPROJECT_DIR/systemd-252/dist/usr/local/lib/pkgconfig
    PKG_GUDEV=$SUBPROJECT_DIR/libgudev/build/dist/lib/pkgconfig
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_GUDEV:$PKG_UDEV:$PKG_GLIB \
    pkg-config --exists --print-errors "gudev-1.0 >= 232"
    ret=$?
    if [ $ret != 0 ]; then 
      echo "not found gudev-1.0"
      if [ -z "$(checkProg name='gperf' args='--version' path=$SUBPROJECT_DIR/gperf-3.1/dist/bin)" ]; then
        echo "not found gperf"
        downloadAndExtract file="gperf-3.1.tar.gz" path="http://ftp.gnu.org/pub/gnu/gperf/gperf-3.1.tar.gz"
        if [ $FAILED -eq 1 ]; then exit 1; fi
        buildMakeProject srcdir="gperf-3.1" prefix="$SUBPROJECT_DIR/gperf-3.1/dist" autogen="skip"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "gperf already found."
      fi

      PKG_LIBCAP=$SUBPROJECT_DIR/libcap/dist/lib64/pkgconfig
      PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBCAP \
      pkg-config --exists --print-errors "libcap >= 2.53" # Or check for sys/capability.h
      ret=$?
      if [ $ret != 0 ]; then
        echo "not found libcap"
        #old link git://git.kernel.org/pub/scm/linux/kernel/git/morgan/libcap.git
        pullOrClone path=https://kernel.googlesource.com/pub/scm/libs/libcap/libcap tag=libcap-2.69
        buildMakeProject srcdir="libcap" prefix="$SUBPROJECT_DIR/libcap/dist" installargs="DESTDIR=$SUBPROJECT_DIR/libcap/dist"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "libcap already found."
      fi

      #TODO build autopoint... (Mint linux)
      PKG_UTIL_LINUX=$SUBPROJECT_DIR/util-linux/dist/lib/pkgconfig
      PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_UTIL_LINUX \
      pkg-config --exists --print-errors "mount >= 2.38.0"
      ret=$?
      if [ $ret != 0 ]; then
        echo "not found mount"
        pullOrClone path=https://github.com/util-linux/util-linux.git tag=v2.38.1
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

      PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_UDEV \
      pkg-config --exists --print-errors "libudev >= 252" # Or check for sys/capability.h
      ret=$?
      if [ $ret != 0 ]; then
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
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dgnu-efi=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtests=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Durlify=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Danalyze=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbpf-framework=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dkernel-install=false"
        downloadAndExtract file="v252.tar.gz" path="https://github.com/systemd/systemd/archive/refs/tags/v252.tar.gz"
        if [ $FAILED -eq 1 ]; then exit 1; fi
        PATH=$PATH:$SUBPROJECT_DIR/gperf-3.1/dist/bin \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBCAP:$PKG_UTIL_LINUX \
        C_INCLUDE_PATH=$SUBPROJECT_DIR/libcap/dist/usr/include \
        LIBRARY_PATH=$SUBPROJECT_DIR/libcap/dist/lib64 \
        buildMesonProject srcdir="systemd-252" prefix="/usr/local" mesonargs="$SYSD_MESON_ARGS" destdir="$SUBPROJECT_DIR/systemd-252/dist"
        if [ $FAILED -eq 1 ]; then exit 1; fi

      else
        echo "libudev already found."
      fi

      pullOrClone path=https://gitlab.gnome.org/GNOME/libgudev.git tag=237
      C_INCLUDE_PATH=$SUBPROJECT_DIR/systemd-252/dist/usr/local/include \
      LIBRARY_PATH=$SUBPROJECT_DIR/libcap/dist/lib64:$SUBPROJECT_DIR/systemd-252/dist/usr/lib \
      PATH=$PATH:$SUBPROJECT_DIR/glib-2.74.1/dist/bin \
      PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_UDEV:$PKG_GLIB \
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
    PKG_PULSE=$SUBPROJECT_DIR/pulseaudio/build/dist/lib/pkgconfig
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_PULSE \
    pkg-config --exists --print-errors "libpulse >= 12.2"
    ret=$?
    if [ $ret != 0 ]; then 
      echo "not found libpulse"

      CMAKE_BIN=$SUBPROJECT_DIR/cmake-3.25.2/build/dist/bin
      PATH=$PATH:$CMAKE_BIN \
      cmake --version
      ret=$?
      if [ $ret != 0 ]; then
        
        downloadAndExtract file="cmake-3.25.2.tar.gz" path="https://github.com/Kitware/CMake/releases/download/v3.25.2/cmake-3.25.2.tar.gz"
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$OPENSSL_PKG \
        buildMakeProject srcdir="cmake-3.25.2" prefix="$SUBPROJECT_DIR/cmake-3.25.2/build/dist"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "cmake already found."
      fi

      PKG_LIBSNDFILE=$SUBPROJECT_DIR/libsndfile/dist/lib/pkgconfig
      PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBSNDFILE \
      pkg-config --exists --print-errors "sndfile >= 1.2.0"
      ret=$?
      if [ $ret != 0 ]; then
        echo "not found sndfile"
        LIBSNDFILE_CMAKEARGS="-DBUILD_EXAMPLES=off"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DBUILD_TESTING=off"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DBUILD_SHARED_LIBS=on"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DINSTALL_PKGCONFIG_MODULE=on"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DINSTALL_MANPAGES=off"
        pullOrClone path="https://github.com/libsndfile/libsndfile.git" tag=1.2.0
        mkdir "libsndfile/build"
        PATH=$PATH:$CMAKE_BIN \
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
      PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBSNDFILE \
      PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_CHECK \
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
    NASM_BIN=$SUBPROJECT_DIR/nasm-2.15.05/dist/bin
    if [ -z "$(checkProg name='nasm' args='--version' path=$NASM_BIN)" ]; then
      echo "not found nasm"
      downloadAndExtract file=nasm-2.15.05.tar.bz2 path=https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.bz2
      buildMakeProject srcdir="nasm-2.15.05" prefix="$SUBPROJECT_DIR/nasm-2.15.05/dist"
      if [ $FAILED -eq 1 ]; then exit 1; fi
    else
        echo "nasm already installed."
    fi

    echo "gst_ret : $gst_ret | gst_libav_ret : $gst_libav_ret | gst_nv_ret : $gst_nv_ret"
    if [ $gst_ret != 0 ] || [ $gst_libav_ret != 0 ] || [ $gst_nv_ret != 0 ]; then
        MESON_PARAMS=""
        if [ $ENABLE_LIBAV -eq 1 ]; then
            echo "LIBAV Feature enabled..."
            
            FFMPEG_BIN=$SUBPROJECT_DIR/FFmpeg/dist/bin
            PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$FFMPEG_PKG \
            pkg-config --exists --print-errors "libavcodec >= 58.20.100"
            ret1=$?
            PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$FFMPEG_PKG \
            pkg-config --exists --print-errors "libavfilter >= 7.40.101"
            ret2=$?
            PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$FFMPEG_PKG \
            pkg-config --exists --print-errors "libavformat >= 58.20.100"
            ret3=$?
            PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$FFMPEG_PKG \
            pkg-config --exists --print-errors "libavutil >= 56.22.100"
            ret4=$?
            PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$FFMPEG_PKG \
            pkg-config --exists --print-errors "libpostproc >= 55.3.100"
            ret5=$?
            PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$FFMPEG_PKG \
            pkg-config --exists --print-errors "libswresample >= 3.3.100"
            ret6=$?
            PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$FFMPEG_PKG \
            pkg-config --exists --print-errors "libswscale >= 5.3.100"
            ret7=$?
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


                pullOrClone path="https://github.com/FFmpeg/FFmpeg.git" tag=n5.0.2
                PATH=$PATH:$NASM_BIN \
                buildMakeProject srcdir="FFmpeg" prefix="$SUBPROJECT_DIR/FFmpeg/dist" configure="$FFMPEG_CONFIGURE_ARGS"
                if [ $FAILED -eq 1 ]; then exit 1; fi
                rm -rf $SUBPROJECT_DIR/FFmpeg/dist/lib/*.so
            else
                echo "FFmpeg already installed."
            fi

            MESON_PARAMS="$MESON_PARAMS -Dlibav=enabled"
        fi

        pullOrClone path="https://gitlab.freedesktop.org/gstreamer/gstreamer.git" tag=$GSTREAMER_LATEST

        # Force disable subproject features
        MESON_PARAMS="$MESON_PARAMS -Dglib:tests=false"
        MESON_PARAMS="$MESON_PARAMS -Dlibdrm:cairo-tests=false"
        MESON_PARAMS="$MESON_PARAMS -Dx264:cli=false"

        # Gstreamer options
        MESON_PARAMS="$MESON_PARAMS -Dbase=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgood=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dbad=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgpl=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:app=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:typefind=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:audiotestsrc=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:videotestsrc=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:playback=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:x11=enabled"
        # MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:xvideo=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:alsa=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:videoconvertscale=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:videorate=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:rawparse=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:pbtypes=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:audioresample=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:audioconvert=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:volume=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:tcp=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:overlaycomposition=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:level=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:v4l2=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:rtsp=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:rtp=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:rtpmanager=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:law=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:autodetect=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:pulse=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:interleave=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:audioparsers=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:udp=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:openh264=enabled"
        if [ $ENABLE_NVCODEC != 0 ]; then
          MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:nvcodec=enabled"
        fi
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:v4l2codecs=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:fdkaac=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:videoparsers=enabled"
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:onvif=enabled"
        # MESON_PARAMS="$MESON_PARAMS -Dugly=enabled"
        # MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-ugly:x264=enabled"

        #Below is required for to workaround https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/1056
        # This is to support v4l2h264enc element with capssetter
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:debugutils=enabled"

        # This is required for the snapshot feature
        MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:png=enabled"

        MESON_PARAMS="-Dauto_features=disabled $MESON_PARAMS"
        MESON_PARAMS="--strip $MESON_PARAMS"

        # Add tinyalsa fallback subproject
        # echo "[wrap-git]" > subprojects/tinyalsa.wrap
        # echo "directory=tinyalsa" >> subprojects/tinyalsa.wrap
        # echo "url=https://github.com/tinyalsa/tinyalsa.git" >> subprojects/tinyalsa.wrap
        # echo "revision=v2.0.0" >> subprojects/tinyalsa.wrap
        # MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:tinyalsa=enabled"

        LIBRARY_PATH=$LD_LIBRARY_PATH:$SUBPROJECT_DIR/systemd-252/dist/usr/lib \
        PATH=$PATH:$SUBPROJECT_DIR/glib-2.74.1/dist/bin:$NASM_BIN \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_GUDEV:$PKG_ALSA:$PKG_PULSE:$PKG_UDEV:$PKG_GLIB:$FFMPEG_PKG:$ALSA_PKG  \
        buildMesonProject srcdir="gstreamer" prefix="$SUBPROJECT_DIR/gstreamer/build/dist" mesonargs="$MESON_PARAMS" builddir="build"
        if [ $FAILED -eq 1 ]; then exit 1; fi
    else
      echo "Gstreamer already built"
    fi
  else
      echo "Latest Gstreamer already installed/built."
  fi
else
    echo "Gstreamer already installed."
fi

################################################################
# 
#    Configure project
# 
################################################################
cd $WORK_DIR
$SCRT_DIR/configure $@