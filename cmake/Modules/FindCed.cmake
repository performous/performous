include(LibFetchMacros)

set(Ced_GIT_VERSION "master")

libfetch_git_pkg(Ced
	REPOSITORY ${SELF_BUILT_GIT_BASE}/compact_enc_det.git
	#https://github.com/google/compact_enc_det.git
	REFERENCE  ${Ced_GIT_VERSION}
	FIND_PATH  compact_enc_det/compact_enc_det.h
)
message(STATUS "Found Google CED ${Ced_VERSION}")
