/*

  Copyright (C) 2016 Gonzalo José Carracedo Carballal

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, version 3.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#include <math.h>
#include <sndfile.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "test.h"

#ifdef _SU_SINGLE_PRECISION
#  define sf_write sf_write_float
#  define SU_REAL_TYPE_STR "float"
#else
#  define sf_write sf_write_double
#  define SU_REAL_TYPE_STR "double"
#endif

SUBOOL
su_sigbuf_pool_helper_dump_matlab(const void *data,
                                  SUSCOUNT    size,
                                  SUBOOL      is_complex,
                                  const char *directory,
                                  const char *name)
{
  char  *filename = NULL;
  SUBOOL result;

  filename = strbuild("%s/%s.m", directory, name);
  if (filename == NULL) {
    SU_ERROR("Memory error while building filename\n");
    goto fail;
  }

  if (is_complex)
    result = su_test_complex_buffer_dump_matlab((const SUCOMPLEX *)data,
                                                size,
                                                filename,
                                                name);
  else
    result =
        su_test_buffer_dump_matlab((const SUFLOAT *)data, size, filename, name);

  free(filename);

  return result;

fail:
  if (filename != NULL)
    free(filename);

  return SU_FALSE;
}

SUBOOL
su_sigbuf_pool_dump_matlab(const su_sigbuf_pool_t *pool)
{
  su_sigbuf_t *this;
  char  *filename = NULL;
  FILE  *fp       = NULL;
  time_t now;

  if ((filename = strbuild("%s.m", pool->name)) == NULL) {
    SU_ERROR("Failed to build main script file name\n");
    goto fail;
  }

  if ((fp = fopen(filename, "w")) == NULL) {
    SU_ERROR("Cannot create main script `%s': %s\n", filename, strerror(errno));
    goto fail;
  }

  free(filename);
  filename = NULL;

  time(&now);

  SU_SYSCALL_ASSERT(
      fprintf(fp,
              "%% Autogenerated MATLAB script for sigbuf pool `%s'\n",
              pool->name));
  SU_SYSCALL_ASSERT(fprintf(fp, "%% File generated on %s", ctime(&now)));

  FOR_EACH_PTR(this, pool, sigbuf)
  {
    if (!su_sigbuf_pool_helper_dump_matlab(this->buffer,
                                           this->size,
                                           this->is_complex,
                                           pool->name,
                                           this->name))
      goto fail;

    SU_SYSCALL_ASSERT(fprintf(fp,
                              "%% %s: %s buffer, %lu elements\n",
                              this->name,
                              this->is_complex ? "complex" : "float",
                              this->size));
    SU_SYSCALL_ASSERT(
        fprintf(fp, "source('%s/%s.m');\n\n", pool->name, this->name));
  }

  fclose(fp);
  fp = NULL;

  return SU_TRUE;

fail:
  if (filename != NULL)
    free(filename);

  if (fp != NULL)
    fclose(fp);

  return SU_FALSE;
}

SUBOOL
su_sigbuf_pool_helper_dump_raw(const void *data,
                               SUSCOUNT    size,
                               SUBOOL      is_complex,
                               const char *directory,
                               const char *name)
{
  char  *filename = NULL;
  SUBOOL result;

  filename = strbuild("%s/%s-%s.raw",
                      directory,
                      name,
                      is_complex ? "complex" : SU_REAL_TYPE_STR);

  if (filename == NULL) {
    SU_ERROR("Memory error while building filename\n");
    goto fail;
  }

  if (is_complex)
    result = su_test_complex_buffer_dump_raw((const SUCOMPLEX *)data,
                                             size,
                                             filename);
  else
    result = su_test_buffer_dump_raw((const SUFLOAT *)data, size, filename);

  free(filename);

  return result;

fail:
  if (filename != NULL)
    free(filename);

  return SU_FALSE;
}

SUBOOL
su_sigbuf_pool_dump_raw(const su_sigbuf_pool_t *pool)
{
  su_sigbuf_t *this;

  FOR_EACH_PTR(this, pool, sigbuf)
  if (!su_sigbuf_pool_helper_dump_raw(this->buffer,
                                      this->size,
                                      this->is_complex,
                                      pool->name,
                                      this->name))
    goto fail;

  return SU_TRUE;

fail:

  return SU_FALSE;
}

SUBOOL
su_sigbuf_pool_helper_dump_wav(const void *data,
                               SUSCOUNT    size,
                               SUSCOUNT    fs,
                               SUBOOL      is_complex,
                               const char *directory,
                               const char *name)
{
  SNDFILE *sf       = NULL;
  char    *filename = NULL;
  SF_INFO  info;
  SUSCOUNT samples = 0;
  SUSCOUNT written = 0;

  info.format     = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
  info.channels   = is_complex ? 2 : 1;
  info.samplerate = fs;

  filename = strbuild("%s/%s.wav", directory, name);
  if (filename == NULL) {
    SU_ERROR("Memory error while building filename\n");
    goto fail;
  }

  if ((sf = sf_open(filename, SFM_WRITE, &info)) == NULL) {
    SU_ERROR("Cannot open `%s' for write: %s\n", filename, sf_strerror(sf));
    goto fail;
  }

  samples = size * info.channels;

  /* UGLY HACK: WE ASSUME THAT A COMPLEX IS TWO DOUBLES */
  if ((written = sf_write(sf, (const SUFLOAT *)data, samples)) != samples) {
    SU_ERROR("Write to `%s' failed: %lu/%lu\n", filename, written, samples);
    goto fail;
  }

  free(filename);
  sf_close(sf);

  return SU_TRUE;

fail:
  if (filename != NULL)
    free(filename);

  if (sf != NULL)
    sf_close(sf);

  return SU_FALSE;
}

SUBOOL
su_sigbuf_pool_dump_wav(const su_sigbuf_pool_t *pool)
{
  su_sigbuf_t *this;

  FOR_EACH_PTR(this, pool, sigbuf)
  {
    if (!su_sigbuf_pool_helper_dump_wav(this->buffer,
                                        this->size,
                                        this->fs,
                                        this->is_complex,
                                        pool->name,
                                        this->name))
      return SU_FALSE;
  }

  return SU_TRUE;
}
