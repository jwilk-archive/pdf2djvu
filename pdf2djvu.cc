#include <iostream>

#include <fcntl.h>
#include <sys/stat.h>

#include "goo/gmem.h"
#include "goo/GooString.h"
#include "GlobalParams.h"
#include "Object.h"
#include "PDFDoc.h"
#include "splash/SplashBitmap.h"
#include "splash/Splash.h"
#include "SplashOutputDev.h"

class Error
{};

class OSError : Error
{};

class MutedSplashOutputDev: public SplashOutputDev
{
public:
  void drawChar(GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode *u, int uLen)
  {
    return;
  }
  virtual GBool useDrawChar() { return gTrue; }

  MutedSplashOutputDev(SplashColorMode colorModeA, int bitmapRowPadA, GBool reverseVideoA, SplashColorPtr paperColorA, GBool bitmapTopDownA = gTrue, GBool allowAntialiasA = gTrue) :
    SplashOutputDev(colorModeA, bitmapRowPadA, reverseVideoA, paperColorA, bitmapTopDownA, allowAntialiasA)
  { }
};

void usage()
{
  std::cerr << "Usage: pdf2djvu <pdf-file>" << std::endl;
  exit(1);
}

void pass_to_stdout(const char *file_name)
{
  int fd = open(file_name, O_RDONLY);
  if (fd == -1)
    throw OSError();
  while (1)
  {
    char buffer[1 << 12];
    int n = read(fd, buffer, sizeof buffer);
    if (n == 0)
      break;
    else if (n < 0)
      throw OSError();
    else
      if (write(STDOUT_FILENO, buffer, n) == -1)
        throw OSError();
  }
}

int main(int argc, char **argv)
{
  if (argc != 2)
    usage();
  GooString file_name(argv[1]);

  globalParams = new GlobalParams(NULL);

  PDFDoc *doc = new PDFDoc(&file_name);
  if (!doc->isOk())
    throw Error();
  
  std::cerr << "About to process: " << doc->getFileName()->getCString() << std::endl;

  SplashColor paper_color;
  for (int i = 0; i < 3; i++)
    paper_color[i] = 0xff;

  SplashOutputDev *out1 = new SplashOutputDev(splashModeRGB8, 4, gFalse, paper_color);
  SplashOutputDev *outm = new MutedSplashOutputDev(splashModeRGB8, 4, gFalse, paper_color);
  out1->startDoc(doc->getXRef());
  outm->startDoc(doc->getXRef());
  int n_pages = doc->getNumPages();
  char djvu_file_name[] = "/tmp/djvu2pdf.XXXXXX";
  int fd = mkstemp(djvu_file_name);
  if (fd == -1)
    throw OSError();
  std::string djvm_command("/usr/bin/djvm -c ");
  std::string *tmp_file_names = new std::string[n_pages + 1];
  djvm_command += djvu_file_name;
  tmp_file_names[0] += djvu_file_name;
  for (int n = 1; n <= n_pages; n++)
  {
    std::cerr << "Page #" << n << ":" << std::endl;
    std::cerr << "- render with text" << std::endl;
    doc->displayPage(out1, n, 300, 300, 0, gTrue, gFalse, gFalse);
    std::cerr << "- render without text" << std::endl;
    doc->displayPage(outm, n, 300, 300, 0, gTrue, gFalse, gFalse);
    std::cerr << "- render done" << std::endl;
    SplashBitmap* bmp1 = out1->getBitmap();
    SplashBitmap* bmpm = outm->getBitmap();
    int width = bmp1->getWidth();
    int height = bmp1->getHeight();
    int row_size = bmp1->getRowSize();
    SplashColorPtr data1 = bmp1->getDataPtr();
    SplashColorPtr datam = bmpm->getDataPtr();
    SplashColorPtr row1, rowm;
    std::cerr << "- sep file" << std::endl;
    char sep_file_name[] = "/tmp/djvu2pdf.XXXXXX";
    int fd = mkstemp(sep_file_name);
    if (fd == -1)
      throw OSError();
    char buffer[1 << 10];
    int len = sprintf(buffer, "R6 %d %d 216\n", width, height);
    write(fd, buffer, len);
    std::cerr << "- rle palette" << std::endl;
    for (int r = 0; r < 6; r++)
    for (int g = 0; g < 6; g++)
    for (int b = 0; b < 6; b++)
    {
      char buffer[] = { 51 * r, 51 * g, 51 * b };
      write(fd, buffer, 3);
    }
    row1 = data1;
    rowm = datam;
    std::cerr << "- generate rle" << std::endl;
    for (int y = 0; y < height; y++)
    {
      SplashColorPtr p1 = row1;
      SplashColorPtr pm = rowm;
      int new_color, color = 0xfff;
      int length = 0;
      for (int x = 0; x < width; x++)
      {
        if (p1[0] != pm[0] || p1[1] != pm[1] || p1[2] != pm[2])
          new_color = (p1[2] / 51) + 6 * ((p1[1] / 51) + 6 * (p1[0] / 51));
        else
          new_color = 0xfff;
        if (color == new_color)
          length++;
        else
        {
          int item = (color << 20) + length;
          for (int i = 0; i < 4; i++)
          {
            char c = item >> ((3 - i) * 8);
            write(fd, &c, 1);
          }
          color = new_color;
          length = 1;
        }
        p1 += 3;
        pm += 3;
      }
      row1 += row_size;
      rowm += row_size;
      int item = (color << 20) + length;
      for (int i = 0; i < 4; i++)
      {
        char c = item >> ((3 - i) * 8);
        if (write(fd, &c, 1) == -1)
          throw OSError();
      }
    }
    std::cerr << "- generate ppm" << std::endl;
    len = sprintf(buffer, "P6 %d %d 255\n", width, height);
    if (write(fd, buffer, len) == -1)
      throw OSError();
    row1 = data1;
    for (int y = 0; y < height; y++)
    {
      if (write(fd, row1, width * 3) == -1)
        throw OSError();
      row1 += row_size;
    }
    if (close(fd) == -1)
      throw OSError();
    char djvu_file_name[] = "/tmp/djvu2pdf.XXXXXX";
    fd = mkstemp(djvu_file_name);
    if (fd < 0)
      throw OSError();
    if (close(fd) == -1)
      throw OSError();
    std::cerr << "- about to call csepdjvu" << std::endl;
    sprintf(buffer, "/usr/bin/csepdjvu %s %s", sep_file_name, djvu_file_name);
    if (system(buffer) == -1)
      throw OSError();
    if (unlink(sep_file_name) != 0)
      throw OSError();
    djvm_command += " ";
    djvm_command += djvu_file_name;
    tmp_file_names[n] += djvu_file_name;
    std::cerr << "- done!" << std::endl;
  }
  std::cerr << "About to call djvm" << std::endl;
  if (system(djvm_command.c_str()) == -1)
    throw OSError();
  std::cerr << "Done!" << std::endl;

  pass_to_stdout(djvu_file_name);
  
  std::cerr << "About to remove temporary files" << std::endl;
  for (int n = 0; n <= n_pages; n++)
    if (unlink(tmp_file_names[n].c_str()) != 0)
      throw OSError();
  std::cerr << "Done!" << std::endl;

  return 0;
}

// vim:ts=2 sw=2 et
