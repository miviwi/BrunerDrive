#include <cstdio>

#include <brdrive.h>

#include <util/primitive.h>

int main(int argc, char *argv[])
{
  printf(
      "Project: %s\n"
      "Author:  %s\n"
      "Version: %s\n"
      "\n"
      "BRDRIVE_ABI_VERSION: %.6llx\n\n",
      brdrive::Project, brdrive::Author, brdrive::Version, BRDRIVE_ABI_VERSION);

  brdrive::i14 i14(0x3FFE);

  brdrive::i9 slice = i14(5, 13);

  printf("i14::Mask=0x%.4llx i14::Sign=0x%.4llx\n"
      "sizeof(i14::StorageType)=%d\n"
      "\n"
      "i14={0x%.4x, %d} i14(5, 13) as i9=(0x%.4x, %d) i14.byte(1)=0x%.2x i14(12)=0x%x\n",
      brdrive::Integer<14>::Mask, brdrive::Integer<14>::Sign, sizeof(brdrive::Integer<14>::StorageType),
      i14.get(), i14.get(), slice.get(), slice.get(), (int)i14.byte(1), (int)i14(12));

  return 0;
}
