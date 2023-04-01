#!/bin/bash
##########################################################
##
## These functions are meant to be pulled into stages
## of the various workflows that require common tasks,
## such as finding the package that was just created.
## This is meant to reduce the size of the workflow
## files, and to utilize a single set of shared code
## among all the workflows, since copying these
## everywhere is error-prone and tedious.
##
## For simplicity, these functions should work on all
## UNIX(like) platforms without any special treatment
## needed to detect OS type, lineage, etc.
## 
##########################################################

## This function finds the package names produced by the
## build, renames the artifact as appropriate, and sets
## the outputs that will be consumed by later actions.
## The first argument is a path to the working drectory,
## usually "$(pwd)". The second argument is a
## bash-compatible regex to search for the package.
## The third argument is the package version as reported
## by ${{ inputs.package_complete_version }}.
## The fourth argument is the OS name, and the fifth
## argument is the OS version, both are optional.
function package_name () {
	WORK_DIR=${1}
	PACKAGE_REGEX=${2}
	PACKAGE_OFFICIAL_VERSION=${3}
	PACKAGE_OS=${4}
	PACKAGE_OS_VERSION=${5}

	if [[ "${PACKAGE_OS}" != "" ]]; then
		PACKAGE_OS_NAME="-${PACKAGE_OS}"
	fi
	if [[ "${PACKAGE_OS_VERSION}" != "" ]]; then
		PACKAGE_OS_VERSION_NAME="_${PACKAGE_OS_VERSION}"
	fi
	PACKAGE_PATH=$(ls ${WORK_DIR}/${PACKAGE_REGEX})
	PACKAGE_NAME=$(basename ${PACKAGE_PATH})
	PACKAGE_SUFFIX=$(echo ${PACKAGE_NAME} | rev | cut -d'.' -f1 | rev)
	NEW_PACKAGE_NAME="Performous-${PACKAGE_OFFICIAL_VERSION}${PACKAGE_OS_NAME}${PACKAGE_OS_VERSION_NAME}.${PACKAGE_SUFFIX}"
	NEW_PACKAGE_PATH="/tmp/${NEW_PACKAGE_NAME}"
	MASTER_NEW_PACKAGE_NAME="Performous-latest${PACKAGE_OS_NAME}${PACKAGE_OS_VERSION_NAME}.${PACKAGE_SUFFIX}"
	MASTER_NEW_PACKAGE_PATH="/tmp/${MASTER_NEW_PACKAGE_NAME}"
	cp ${PACKAGE_PATH} ${MASTER_NEW_PACKAGE_PATH}
	cp ${PACKAGE_PATH} ${NEW_PACKAGE_PATH}
	ARTIFACT_NAME=$(basename ${NEW_PACKAGE_NAME})
	MASTER_ARTIFACT_NAME=$(basename ${MASTER_NEW_PACKAGE_NAME})
	echo "ARTIFACT_PATH=${NEW_PACKAGE_PATH}" >> ${GITHUB_ENV}
	echo "ARTIFACT_NAME=${NEW_PACKAGE_NAME}" >> ${GITHUB_ENV}
	echo "MASTER_ARTIFACT_PATH=${MASTER_NEW_PACKAGE_PATH}" >> ${GITHUB_ENV}
	echo "MASTER_ARTIFACT_NAME=${MASTER_NEW_PACKAGE_NAME}" >> ${GITHUB_ENV}
}
