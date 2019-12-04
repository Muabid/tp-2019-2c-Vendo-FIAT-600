/*
 * operations.h
 *
 *  Created on: 28 sep. 2019
 *      Author: utnso
 */

#ifndef OPERATIONS_CLIENT_H_
#define OPERATIONS_CLIENT_H_
#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define T_FILE 1
#define T_DIR 2
/*
 * Link donde están las definiciones de las funciones y lo que hacen:
 * 	https://gist.github.com/bkmeneguello/5884492
 *
 * Link donde hay un ejemplo de implementación:
 *
 */

static int do_getattr(const char *path, struct stat *st);
static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int do_read(const char *, char *, size_t, off_t, struct fuse_file_info *);
static int do_unlink(const char *path);
static int do_mkdir(const char *path, mode_t mode);
static int do_opendir(const char *path, struct fuse_file_info *fi);
static int do_rmdir(const char *path);
static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi);
static int do_mknod(const char *path, mode_t mode, dev_t rdev);
static int do_access(const char* path, int mask);
static int do_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi);
static int do_setxattr(const char *path, const char *name,const void *value, size_t size, int flags);
static int do_utimens(const char* path, const struct timespec ts[2]);
static int do_trucate(const char *filename, off_t offset);
static int do_rename(const char* old_path,const char* new_path);
static int do_release(const char *, struct fuse_file_info *);
/*
 * faltan: , , , acces, chmod,chown
 */


#endif /* OPERATIONS_CLIENT_H_ */
