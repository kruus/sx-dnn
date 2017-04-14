#!/bin/bash
# vim: et ts=4 sw=4
ORIGINAL_CMD="$0 $*"
DOVANILLA="y"
DOJIT=0
DOTEST=0
DODEBUG="n"
DODOC="y"
usage() {
    echo "$0 usage:"
    #head -n 30 "$0" | grep "^[^#]*.)\ #"
    awk '/getopts/{flag=1;next} /done/{flag=0} flag&&/^[^#]+) #/; flag&&/^ *# /' $0
    echo "Example: full test run, debug compile---     $0 -dtt"
    exit 0
}
while getopts ":htvijdq" arg; do
    #echo "arg = ${arg}, OPTIND = ${OPTIND}, OPTARG=${OPTARG}"
    case $arg in
        t) # [0] increment test level: (1) examples, (2) tests (longer), ...
            # 1: examples ~  1 min  (jit), 15 min (vanilla)
            # 2: test_*   ~ 10 mins (jit), 10 hrs (vanilla)
            DOTEST=$(( DOTEST + 1 ))
            ;;
        v) # [yes] (src/vanilla C/C++ only: no src/cpu JIT assembler)
            DOVANILLA="y"; DOJIT=0
            ;;
    i | j) # [no] Intel JIT (src/cpu JIT assembly version)
            DOVANILLA="n"; DOJIT=100 # 100 means all JIT funcs enabled
            ;;
        d) # [no] debug release
            DODEBUG="y"
            ;;
        q) # quick: skip doxygen docs [default: run doxygen if build OK]
            DODOC="n"
            ;;
    h | *) # help
            usage
            ;;
    esac
    shift
done
timeoutPID() {
    PID="$1"
    timeout="$2"
    interval=1
    delay=1
    (
        ((t = timeout))

        while ((t > 0)); do
            sleep $interval
            kill -0 $$ || exit 0
            ((t -= interval))
        done

        # Be nice, post SIGTERM first.
        # The 'exit 0' below will be executed if any preceeding command fails.
        kill -s SIGTERM $$ && kill -0 $$ || exit 0
        sleep $delay
        kill -s SIGKILL $$
    ) 2> /dev/null &
}
(
    echo "DOVANILLA $DOVANILLA"
    echo "DOJIT     $DOJIT"
    echo "DOTEST    $DOTEST"
    echo "DODEBUG   $DODEBUG"
    echo "DODOC     $DODOC"
    if [ -d build ]; then rm -rf build.bak && mv -v build build.bak; fi
    if [ -d install ]; then rm -rf install.bak && mv -v install install.bak; fi
    mkdir build
    cd build
    #
    CMAKEOPT=''
    CMAKEOPT="${CMAKEOPT} -DCMAKE_CCXX_FLAGS=-DJITFUNCS=${DOJIT}"
    if [ "$DOVANILLA" == "y" ]; then
        CMAKEOPT="${CMAKEOPT} -DTARGET_VANILLA=ON"
    fi
    if [ "$DODEBUG" == "y" ]; then
        CMAKEOPT="${CMAKEOPT} -DCMAKE_BUILD_TYPE=Debug"
        CMAKEOPT="${CMAKEOPT} -DCMAKE_INSTALL_PREFIX=../install-dbg"
    else
        #CMAKEOPT="${CMAKEOPT} -DCMAKE_BUILD_TYPE=Release"
        CMAKEOPT="${CMAKEOPT} -DCMAKE_BUILD_TYPE=RelWithDebInfo"
        CMAKEOPT="${CMAKEOPT} -DCMAKE_INSTALL_PREFIX=../install"
    fi
    CMAKEOPT="${CMAKEOPT} -DFAIL_WITHOUT_MKL=ON"
    # Without MKL, unit tests take **forever**
    #    TODO: cblas / mathkeisan alternatives?
    BUILDOK="n"
    rm -f ./stamp-BUILDOK
    echo "cmake ${CMAKEOPT} .."
    cmake ${CMAKEOPT} .. && \
	    make VERBOSE=1 -j8 && \
        BUILDOK="y"
    if [ "$BUILDOK" == "y" ]; then
        echo "DOVANILLA $DOVANILLA"
        echo "DOJIT     $DOJIT"
        echo "DOTEST    $DOTEST"
        echo "DODEBUG   $DODEBUG"
        echo "DODOC     $DODOC"
        # Whatever you are currently debugging can go here ...
        { echo "api-io-c                ..."; time tests/api-io-c || BUILDOK="n"; }
        if [ "$DOTEST" == 0 -a "$DOJIT" -gt 0 ]; then # this is fast ONLY with JIT (< 5 secs vs > 5 mins)
            { echo "simple-training-net-cpp ..."; time examples/simple-training-net-cpp || BUILDOK="n"; }
        fi
    fi
    if [ "$BUILDOK" == "y" ]; then
        touch ./stamp-BUILDOK
        if [ "$DODOC" == "y" ]; then
            echo "Build OK... Doxygen (please be patient)"
            make doc >& ../doxygen.log
        fi
    fi
) 2>&1 | tee build.log
BUILDOK="n"; if [ -f build/stamp-BUILDOK ]; then BUILDOK="y"; fi # check last thing produced for OK build
if [ "$BUILDOK" == "y" ]; then
    (
        cd build
        { echo "Installing ..."; make install; }
    ) 2>&1 >> build.log
    if [ "$DOTEST" -gt 0 ]; then
        echo "Testing ..."
        rm -f test1.log test2.log
        (cd build && ARGS='-VV -E .*test_.*' /usr/bin/time -v make test) 2>&1 | tee test1.log
        if [ "$DOTEST" -gt 1 ]; then
            (cd build && ARGS='-VV -N' make test \
                && ARGS='-VV -R .*test_.*' /usr/bin/time -v make test) 2>&1 | tee test2.log
        fi
        echo "Tests done"
    fi
else
    echo "Build NOT OK..."
fi
echo "DOVANILLA=${DOVANILLA}, DOJIT=${DOJIT}, DOTEST=${DOTEST}, DODEBUG=${DODEBUG}, DODOC=${DODOC}"
if [ "$DOTEST" == "y" ]; then
    LOGDIR="log-${DOVANILLA}${DOJIT}${DOTEST}${DODEBUG}${DODOC}"
    echo "LOGDIR:       ${LOGDIR}" 2>&1 >> build.log
fi
if [ "$DOTEST" == "y" ]; then
    if [ -d "${LOGDIR}" ]; then mv "${LOGDIR}" "${LOGDIR}.bak"; fi
    mkdir ${LOGDIR}
    for f in build.log test1.log test2.log; do
        cp -av "${f}" "${LOGDIR}/"
    done
fi
echo "FINISHED:     $ORIGINAL_CMD" 2>&1 >> build.log
# for a debug compile  --- FIXME
#(cd build && ARGS='-VV -R .*simple_training-net-cpp' /usr/bin/time -v make test) 2>&1 | tee test1-dbg.log
#(cd build && ARGS='-VV -R .*simple_training-net-cpp' valgrind make test) 2>&1 | tee test1-valgrind.log
