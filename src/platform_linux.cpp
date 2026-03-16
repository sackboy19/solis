#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "core.h"

namespace sol {

static B32 ReadEntireFile(Arena *arena, CString path) {
	int fd = open(path, O_RDONLY);
	struct stat st;
	if (fstat(fd, &st) != 0) {
		close(fd);
		return false;
	}

	void* buf = ArenaPushEx(arena, st.st_size, alignof(char));
	read(fd, buf, st.st_size);
	close(fd);

	return true;
}

} // namespace sol