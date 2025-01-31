DBGFLAGS = -O2 -g0 -DNDEBUG
ifeq ($(DEBUG),1)
 DBGFLAGS = -O0 -g3 -D_DEBUG
endif
ifeq ($(PROFILE),1)
 DBGFLAGS = -O2 -g3
endif

ifeq ($(strip $(DIR)),)
 PWD=$(shell pwd) 
 DIR=$(shell basename $(PWD))
endif
OBJECT=$(DIR).o
EXECUTABLE=../flowplugins/$(DIR).so


CCOMPILER=g++ -std=c++11 -fno-exceptions -fvisibility=hidden -Wall -fPIC -DPIC -Wno-unused-local-typedefs -Wno-unused-result $(DBGFLAGS) -D_REENTRANT -DENV_LINUX -DOAPC_EXT_EXPORTS

SYSINCLUDES= -I.  -I../BeamSDK/include/ -I../plugins/ -I../liboapc/
	
SYSLIBRARIES= -loapc
	
LINK=g++ -shared
	
default: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECT) $(CUSTOBJECTS)
	$(LINK) -o $(EXECUTABLE) $(OBJECT) $(CUSTOBJECTS) \
	-Wl,-whole-archive \
	$(SYSLIBRARIES) $(CUSTLIBRARIES) \
	-Wl,-no-whole-archive
	sudo cp $(EXECUTABLE) /usr/lib/openapc/flowplugins/ &
	sudo cp $(EXECUTABLE) /usr/lib64/openapc/flowplugins/ &

%.o: %.cpp
	$(CCOMPILER) $(SYSINCLUDES) $(CUSTINCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECT) $(CUSTOBJECTS) $(EXECUTABLE)
