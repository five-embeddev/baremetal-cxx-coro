##
# Project Title
#
# @file
# @version 0.1

PUML_FILES=$(wildcard *.puml)
SVG_FILES=$(PUML_FILES:%.puml=%.svg)
PNG_FILES=$(PUML_FILES:%.puml=%.png)
PLANTUML_URL=https://github.com/plantuml/plantuml/releases/download/v1.2024.6/plantuml-1.2024.6.jar
PLANTUML_JAR=plantuml-1.2024.6.jar

all : ${SVG_FILES} ${PNG_FILES}

plantuml : ${PLANTUML_JAR}
	java -jar ${PLANTUML_JAR} -h

%.svg : %.puml ${PLANTUML_JAR}
	java -jar ${PLANTUML_JAR} -tsvg $<

%.png : %.puml ${PLANTUML_JAR}
	java -jar ${PLANTUML_JAR} -tpng $<

${PLANTUML_JAR} :
	wget ${PLANTUML_URL}

# end
