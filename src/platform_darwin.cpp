//
// platform_darwin.cpp
// Darwin (macOS, iOS) platform layer for the Solis compiler
//
// Author: Sackboy
// License: GNU GPLv3.0
//

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "core.h"

namespace sol {

static Result<String> ReadEntireFile(Arena *arena, CString path) {
	int fd = open(path, O_RDONLY);
	struct stat st;
	if (fstat(fd, &st) != 0) {
		close(fd);
		return Failure(Error::COULD_NOT_OPEN_FILE);
	}

	void *buf = ArenaPushEx(arena, st.st_size+1, alignof(char));
	read(fd, buf, st.st_size);
	close(fd);

	return Success(MakeStringEx(buf, st.st_size));
}

} // namespace sol