#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/5c0/appio.o \
	${OBJECTDIR}/_ext/d2a1ad61/apps.o \
	${OBJECTDIR}/_ext/d2a1ad61/audioSink.o \
	${OBJECTDIR}/_ext/d2a1ad61/bmp180App.o \
	${OBJECTDIR}/_ext/d2a1ad61/dcf77.o \
	${OBJECTDIR}/_ext/d2a1ad61/flasher.o \
	${OBJECTDIR}/_ext/d2a1ad61/spiram.o \
	${OBJECTDIR}/_ext/5c0/bus_server.o \
	${OBJECTDIR}/_ext/e5d2b957/hw_raspbian.o \
	${OBJECTDIR}/_ext/e5d2b957/ip_raspbian.o \
	${OBJECTDIR}/_ext/e5d2b957/tick_raspbian.o \
	${OBJECTDIR}/_ext/e5d2b957/uart_raspbian.o \
	${OBJECTDIR}/_ext/5c0/ip_client.o \
	${OBJECTDIR}/_ext/5c0/main.o \
	${OBJECTDIR}/_ext/5c0/persistence.o \
	${OBJECTDIR}/_ext/5c0/protocol.o \
	${OBJECTDIR}/_ext/5c0/rs485.o \
	${OBJECTDIR}/_ext/5c0/sinks.o \
	${OBJECTDIR}/_ext/828e6f31/bmp180.o \
	${OBJECTDIR}/_ext/828e6f31/dht11.o \
	${OBJECTDIR}/_ext/828e6f31/digio.o \
	${OBJECTDIR}/_ext/828e6f31/displaySink.o \
	${OBJECTDIR}/_ext/828e6f31/halfduplex.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lm

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/netmaster

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/netmaster: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/netmaster ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/5c0/appio.o: ../appio.c
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/appio.o ../appio.c

${OBJECTDIR}/_ext/d2a1ad61/apps.o: ../apps/apps.c
	${MKDIR} -p ${OBJECTDIR}/_ext/d2a1ad61
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2a1ad61/apps.o ../apps/apps.c

${OBJECTDIR}/_ext/d2a1ad61/audioSink.o: ../apps/audioSink.c
	${MKDIR} -p ${OBJECTDIR}/_ext/d2a1ad61
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2a1ad61/audioSink.o ../apps/audioSink.c

${OBJECTDIR}/_ext/d2a1ad61/bmp180App.o: ../apps/bmp180App.c
	${MKDIR} -p ${OBJECTDIR}/_ext/d2a1ad61
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2a1ad61/bmp180App.o ../apps/bmp180App.c

${OBJECTDIR}/_ext/d2a1ad61/dcf77.o: ../apps/dcf77.c
	${MKDIR} -p ${OBJECTDIR}/_ext/d2a1ad61
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2a1ad61/dcf77.o ../apps/dcf77.c

${OBJECTDIR}/_ext/d2a1ad61/flasher.o: ../apps/flasher.c
	${MKDIR} -p ${OBJECTDIR}/_ext/d2a1ad61
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2a1ad61/flasher.o ../apps/flasher.c

${OBJECTDIR}/_ext/d2a1ad61/spiram.o: ../apps/spiram.c
	${MKDIR} -p ${OBJECTDIR}/_ext/d2a1ad61
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2a1ad61/spiram.o ../apps/spiram.c

${OBJECTDIR}/_ext/5c0/bus_server.o: ../bus_server.c
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/bus_server.o ../bus_server.c

${OBJECTDIR}/_ext/e5d2b957/hw_raspbian.o: ../hardware/hw_raspbian.c
	${MKDIR} -p ${OBJECTDIR}/_ext/e5d2b957
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/e5d2b957/hw_raspbian.o ../hardware/hw_raspbian.c

${OBJECTDIR}/_ext/e5d2b957/ip_raspbian.o: ../hardware/ip_raspbian.c
	${MKDIR} -p ${OBJECTDIR}/_ext/e5d2b957
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/e5d2b957/ip_raspbian.o ../hardware/ip_raspbian.c

${OBJECTDIR}/_ext/e5d2b957/tick_raspbian.o: ../hardware/tick_raspbian.c
	${MKDIR} -p ${OBJECTDIR}/_ext/e5d2b957
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/e5d2b957/tick_raspbian.o ../hardware/tick_raspbian.c

${OBJECTDIR}/_ext/e5d2b957/uart_raspbian.o: ../hardware/uart_raspbian.c
	${MKDIR} -p ${OBJECTDIR}/_ext/e5d2b957
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/e5d2b957/uart_raspbian.o ../hardware/uart_raspbian.c

${OBJECTDIR}/_ext/5c0/ip_client.o: ../ip_client.c
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/ip_client.o ../ip_client.c

${OBJECTDIR}/_ext/5c0/main.o: ../main.c
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/main.o ../main.c

${OBJECTDIR}/_ext/5c0/persistence.o: ../persistence.c
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/persistence.o ../persistence.c

${OBJECTDIR}/_ext/5c0/protocol.o: ../protocol.c
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/protocol.o ../protocol.c

${OBJECTDIR}/_ext/5c0/rs485.o: ../rs485.c
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/rs485.o ../rs485.c

${OBJECTDIR}/_ext/5c0/sinks.o: ../sinks.c
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/sinks.o ../sinks.c

${OBJECTDIR}/_ext/828e6f31/bmp180.o: ../sinks/bmp180.c
	${MKDIR} -p ${OBJECTDIR}/_ext/828e6f31
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/828e6f31/bmp180.o ../sinks/bmp180.c

${OBJECTDIR}/_ext/828e6f31/dht11.o: ../sinks/dht11.c
	${MKDIR} -p ${OBJECTDIR}/_ext/828e6f31
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/828e6f31/dht11.o ../sinks/dht11.c

${OBJECTDIR}/_ext/828e6f31/digio.o: ../sinks/digio.c
	${MKDIR} -p ${OBJECTDIR}/_ext/828e6f31
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/828e6f31/digio.o ../sinks/digio.c

${OBJECTDIR}/_ext/828e6f31/displaySink.o: ../sinks/displaySink.c
	${MKDIR} -p ${OBJECTDIR}/_ext/828e6f31
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/828e6f31/displaySink.o ../sinks/displaySink.c

${OBJECTDIR}/_ext/828e6f31/halfduplex.o: ../sinks/halfduplex.c
	${MKDIR} -p ${OBJECTDIR}/_ext/828e6f31
	${RM} "$@.d"
	$(COMPILE.c) -g -DDEBUG -D_CONF_RASPBIAN -D__USE_BSD -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/828e6f31/halfduplex.o ../sinks/halfduplex.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
