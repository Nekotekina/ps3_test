#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include <cell/sysmodule.h>
#include <sysutil/sysutil_sysparam.h>
#include <sys/paths.h>
#include <cell/cell_fs.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/spu_thread.h>
#include <sys/raw_spu.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>
#include <ppu_intrinsics.h>
#include <altivec.h>
#include <float.h>

void sample_sysutil_callback(uint64_t status, uint64_t param, void *userdata)
{
	switch (status)
	{
	case CELL_SYSUTIL_REQUEST_EXITGAME:
		exit(0);
		break;
	case CELL_SYSUTIL_DRAWING_BEGIN:
		break;
	case CELL_SYSUTIL_DRAWING_END:
		break;
	default:
		printf("sysutil callback : 0x%llx\n", status);
		break;
	}
}

FILE* results;

void test_range(uint64_t first, uint64_t last)
{
	printf("Testing range 0x%llx-0x%llx\n", first, last);

	uint64_t last_res = 123, last_base = 0;

	for (uint64_t base = first; base <= last; base++)
	{
		double arg = 0, res;
		uint64_t _data = base << 32, _out;
		memcpy(&arg, &_data, 8);
		__asm__ ("frsqrte %0,%1" : "=f"(res) : "f"(arg));
		memcpy(&_out, &res, 8);
		if (base == first)
		{
			last_res = _out;
			last_base = base;
		}
		else if (last_res != _out)
		{
			fprintf(results, "0x%016llx..0x%016llx = 0x%016llx\n", last_base << 32, (base - 1) << 32, last_res);
			last_res = _out;
			last_base = base;
		}
	}

	fprintf(results, "0x%016llx..0x%016llx = 0x%016llx\n", last_base << 32, last << 32, last_res);
}

int main(int argc, char* argv[])
{
	int ret;

	cellSysmoduleLoadModule(CELL_SYSMODULE_IO);
	cellSysmoduleLoadModule(CELL_SYSMODULE_FS);

	ret = cellSysutilRegisterCallback(0, sample_sysutil_callback, NULL);
	if (ret != 0)
	{
		printf("cellSysutilRegisterCallback() failed (0x%x)\n", ret);
		return ret;
	}

	// Initialize result file
	{
		char fname[1024] = {0};
		strcpy(fname, argv[0]);
		strcat(fname, ".txt");
		results = fopen(fname, "wb");

		if (results == NULL)
		{
			printf("Failed to open file %s.\n", fname);
		}
	}

	test_range(0x00000000, 0x7ff00001);
	test_range(0x7ff80000, 0x7ff80001);
	test_range(0x80000000, 0xfff00001);
	test_range(0xfff80000, 0xfff80001);
	printf("Done.\n");
	return 0;
}
