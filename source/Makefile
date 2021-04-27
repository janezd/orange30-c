MODULENAME=ORANGE

include ../makefile.defs

all:	objdir $(OLD)/orange.so



#obj/daxpy.o : blas/daxpy.c 
#	$(CCOMPILER) $(COMPILEOPTIONS) -c blas/daxpy.c  -o obj/daxpy.o
#	
#obj/ddot.o : blas/ddot.c 
#	$(CCOMPILER) $(COMPILEOPTIONS) -c blas/ddot.c  -o obj/ddot.o
#
#obj/dnrm2.o : blas/dnrm2.c 
#	$(CCOMPILER) $(COMPILEOPTIONS) -c blas/dnrm2.c  -o obj/dnrm2.o
#
#obj/dscal.o : blas/dscal.c 
#	$(CCOMPILER) $(COMPILEOPTIONS) -c blas/dscal.c  -o obj/dscal.o
#
BLAS_OBJECTS = #obj/daxpy.o obj/ddot.o obj/dnrm2.o obj/dscal.o

ORANGE_OBJECTS=basicattrstat.o basketfeeder.o classifier.o constantclassifier.o contdistribution.o contingency.o continuousvariable.o converts.o discdistribution.o discretevariable.o distribution.o domain.o domainbasicattrstat.o domaindistributions.o errors.o example.o examplesdistance.o exampletable.o exampletablereader.o findnearest.o gaussiandistribution.o knn.o learner.o majoritylearner.o module.o orange.o orattributedvector.o orvector.o probabilityestimator.o progarguments.o pyvalue.o randomclassifier.o randomgenerator.o stringvariable.o variable.o 


common.hpp.gch: common.hpp
	$(COMPILER) $(COMPILEOPTIONS) common.hpp


$(OLD)/orange.so:	$(ORANGE_OBJECTS) $(BLAS_OBJECTS)
	$(LINKER) $(ORANGE_OBJECTS) $(BLAS_OBJECTS) $(LINKOPTIONS) -o $(OLD)/orange.so
ifeq ($(OS), Darwin)
	install_name_tool -id $(DESTDIR)/orange.so $(OLD)/orange.so
endif
	cd $(OLD); ln -sf orange.so $(LIBRARY_FILE)

clean: cleantemp
	rm -f r_imports.ipp r_imports.jpp
	rm -f $(OLD)/orange.so $(OLD)/liborange.so

depend:
	makedepend
% makedepend -I . -I ../include -I ppp -I px *.cpp

# DO NOT DELETE


