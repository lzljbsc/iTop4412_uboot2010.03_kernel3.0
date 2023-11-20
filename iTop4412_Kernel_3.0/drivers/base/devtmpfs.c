/*
 * devtmpfs - kernel-maintained tmpfs-based /dev
 *
 * Copyright (C) 2009, Kay Sievers <kay.sievers@vrfy.org>
 *
 * During bootup, before any driver core device is registered,
 * devtmpfs, a tmpfs-based filesystem is created. Every driver-core
 * device which requests a device node, will add a node in this
 * filesystem.
 * By default, all devices are named after the the name of the
 * device, owned by root and have a default mode of 0600. Subsystems
 * can overwrite the default setting if needed.
 */

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/mount.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/shmem_fs.h>
#include <linux/ramfs.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <linux/init_task.h>
#include <linux/slab.h>

/* 文件系统实例，是devtmpfs 文件系统挂载点 */
static struct vfsmount *dev_mnt;

/* devtmpfs 挂载 
 * 是否将 devtmpfs 挂载到系统目录，即 /dev */
#if defined CONFIG_DEVTMPFS_MOUNT
static int mount_dev = 1;
#else
static int mount_dev;
#endif

static DEFINE_MUTEX(dirlock);

/* 启动参数，用来控制在内核挂载根文件系统后是否挂载 devtmpfs 
 * 如果uboot传入了 devtmpfs.mount=0 则不进行挂载 */
static int __init mount_param(char *str)
{
	mount_dev = simple_strtoul(str, NULL, 0);
	return 1;
}
__setup("devtmpfs.mount=", mount_param);

static struct dentry *dev_mount(struct file_system_type *fs_type, int flags,
		      const char *dev_name, void *data)
{
#ifdef CONFIG_TMPFS
	return mount_single(fs_type, flags, data, shmem_fill_super);
#else
	return mount_single(fs_type, flags, data, ramfs_fill_super);
#endif
}

static struct file_system_type dev_fs_type = {
	.name = "devtmpfs",
	.mount = dev_mount,
	.kill_sb = kill_litter_super,
};

#ifdef CONFIG_BLOCK
static inline int is_blockdev(struct device *dev)
{
	return dev->class == &block_class;
}
#else
static inline int is_blockdev(struct device *dev) { return 0; }
#endif

/* 创建目录 */
static int dev_mkdir(const char *name, mode_t mode)
{
	struct nameidata nd;
	struct dentry *dentry;
	int err;

    /* 查找父目录，如果父目录不存在，则返回错误 */
	err = vfs_path_lookup(dev_mnt->mnt_root, dev_mnt,
			      name, LOOKUP_PARENT, &nd);
	if (err)
		return err;

	dentry = lookup_create(&nd, 1);
	if (!IS_ERR(dentry)) {
		err = vfs_mkdir(nd.path.dentry->d_inode, dentry, mode);
		if (!err)
			/* mark as kernel-created inode */
			dentry->d_inode->i_private = &dev_mnt;
		dput(dentry);
	} else {
		err = PTR_ERR(dentry);
	}

	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	path_put(&nd.path);
	return err;
}

/* 创建路径 */
static int create_path(const char *nodepath)
{
	int err;

    /* 直接创建路径，如果创建失败
     * 则认为是有多层目录（多层目录不能直接创建）
     * 按照目录层次逐个创建 */
	mutex_lock(&dirlock);
	err = dev_mkdir(nodepath, 0755);
	if (err == -ENOENT) {
		char *path;
		char *s;

		/* parent directories do not exist, create them */
		path = kstrdup(nodepath, GFP_KERNEL);
		if (!path) {
			err = -ENOMEM;
			goto out;
		}
		s = path;
		for (;;) {
			s = strchr(s, '/');
			if (!s)
				break;
			s[0] = '\0';
			err = dev_mkdir(path, 0755);
			if (err && err != -EEXIST)
				break;
			s[0] = '/';
			s++;
		}
		kfree(path);
	}
out:
	mutex_unlock(&dirlock);
	return err;
}

/* devtmpfs 创建节点
 * 被 device_add 调用*/
int devtmpfs_create_node(struct device *dev)
{
	const char *tmp = NULL;
	const char *nodename;
	const struct cred *curr_cred;
	mode_t mode = 0;
	struct nameidata nd;
	struct dentry *dentry;
	int err;

    /* devtmpfs 必须正确挂载 */
	if (!dev_mnt)
		return 0;

    /* 获取设备的节点名称，可能包含 '/' */
	nodename = device_get_devnode(dev, &mode, &tmp);
	if (!nodename)
		return -ENOMEM;

    /* 默认的权限 0600 */
	if (mode == 0)
		mode = 0600;
    /* 设置设备类型标志， 块设备 / 字符设备 */
	if (is_blockdev(dev))
		mode |= S_IFBLK;
	else
		mode |= S_IFCHR;

    /* 权限相关，暂不分析 */
	curr_cred = override_creds(&init_cred);

    /* 尝试找一下需要创建的设备路径，找不到就创建一下 */
	err = vfs_path_lookup(dev_mnt->mnt_root, dev_mnt,
			      nodename, LOOKUP_PARENT, &nd);
	if (err == -ENOENT) {
		create_path(nodename);
		err = vfs_path_lookup(dev_mnt->mnt_root, dev_mnt,
				      nodename, LOOKUP_PARENT, &nd);
	}
	if (err)
		goto out;

    /* 找一下子节点的 dentry 没有则创建 */
	dentry = lookup_create(&nd, 0);
	if (!IS_ERR(dentry)) {
        /* 创建节点 */
		err = vfs_mknod(nd.path.dentry->d_inode,
				dentry, mode, dev->devt);
		if (!err) {
			struct iattr newattrs;

			/* fixup possibly umasked mode */
			newattrs.ia_mode = mode;
			newattrs.ia_valid = ATTR_MODE;
			mutex_lock(&dentry->d_inode->i_mutex);
			notify_change(dentry, &newattrs);
			mutex_unlock(&dentry->d_inode->i_mutex);

			/* mark as kernel-created inode */
			dentry->d_inode->i_private = &dev_mnt;
		}
		dput(dentry);
	} else {
		err = PTR_ERR(dentry);
	}

	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	path_put(&nd.path);
out:
	kfree(tmp);
	revert_creds(curr_cred);
	return err;
}

static int dev_rmdir(const char *name)
{
	struct nameidata nd;
	struct dentry *dentry;
	int err;

	err = vfs_path_lookup(dev_mnt->mnt_root, dev_mnt,
			      name, LOOKUP_PARENT, &nd);
	if (err)
		return err;

	mutex_lock_nested(&nd.path.dentry->d_inode->i_mutex, I_MUTEX_PARENT);
	dentry = lookup_one_len(nd.last.name, nd.path.dentry, nd.last.len);
	if (!IS_ERR(dentry)) {
		if (dentry->d_inode) {
			if (dentry->d_inode->i_private == &dev_mnt)
				err = vfs_rmdir(nd.path.dentry->d_inode,
						dentry);
			else
				err = -EPERM;
		} else {
			err = -ENOENT;
		}
		dput(dentry);
	} else {
		err = PTR_ERR(dentry);
	}

	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	path_put(&nd.path);
	return err;
}

static int delete_path(const char *nodepath)
{
	const char *path;
	int err = 0;

	path = kstrdup(nodepath, GFP_KERNEL);
	if (!path)
		return -ENOMEM;

	mutex_lock(&dirlock);
	for (;;) {
		char *base;

		base = strrchr(path, '/');
		if (!base)
			break;
		base[0] = '\0';
		err = dev_rmdir(path);
		if (err)
			break;
	}
	mutex_unlock(&dirlock);

	kfree(path);
	return err;
}

static int dev_mynode(struct device *dev, struct inode *inode, struct kstat *stat)
{
	/* did we create it */
	if (inode->i_private != &dev_mnt)
		return 0;

	/* does the dev_t match */
	if (is_blockdev(dev)) {
		if (!S_ISBLK(stat->mode))
			return 0;
	} else {
		if (!S_ISCHR(stat->mode))
			return 0;
	}
	if (stat->rdev != dev->devt)
		return 0;

	/* ours */
	return 1;
}

/* 溢出 devtmpfs 下的设备节点，
 * 与 devtmpfs_create_node 相反的流程 */
int devtmpfs_delete_node(struct device *dev)
{
	const char *tmp = NULL;
	const char *nodename;
	const struct cred *curr_cred;
	struct nameidata nd;
	struct dentry *dentry;
	struct kstat stat;
	int deleted = 1;
	int err;

	if (!dev_mnt)
		return 0;

	nodename = device_get_devnode(dev, NULL, &tmp);
	if (!nodename)
		return -ENOMEM;

	curr_cred = override_creds(&init_cred);
	err = vfs_path_lookup(dev_mnt->mnt_root, dev_mnt,
			      nodename, LOOKUP_PARENT, &nd);
	if (err)
		goto out;

	mutex_lock_nested(&nd.path.dentry->d_inode->i_mutex, I_MUTEX_PARENT);
	dentry = lookup_one_len(nd.last.name, nd.path.dentry, nd.last.len);
	if (!IS_ERR(dentry)) {
		if (dentry->d_inode) {
			err = vfs_getattr(nd.path.mnt, dentry, &stat);
			if (!err && dev_mynode(dev, dentry->d_inode, &stat)) {
				struct iattr newattrs;
				/*
				 * before unlinking this node, reset permissions
				 * of possible references like hardlinks
				 */
				newattrs.ia_uid = 0;
				newattrs.ia_gid = 0;
				newattrs.ia_mode = stat.mode & ~0777;
				newattrs.ia_valid =
					ATTR_UID|ATTR_GID|ATTR_MODE;
				mutex_lock(&dentry->d_inode->i_mutex);
				notify_change(dentry, &newattrs);
				mutex_unlock(&dentry->d_inode->i_mutex);
				err = vfs_unlink(nd.path.dentry->d_inode,
						 dentry);
				if (!err || err == -ENOENT)
					deleted = 1;
			}
		} else {
			err = -ENOENT;
		}
		dput(dentry);
	} else {
		err = PTR_ERR(dentry);
	}
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);

	path_put(&nd.path);
	if (deleted && strchr(nodename, '/'))
		delete_path(nodename);
out:
	kfree(tmp);
	revert_creds(curr_cred);
	return err;
}

/*
 * If configured, or requested by the commandline, devtmpfs will be
 * auto-mounted after the kernel mounted the root filesystem.
 */
/* 这个函数是被内核自动调用的
 * 用于在内核已经启动，挂载了根文件系统后，
 * 将 devtmpfs 挂载到 /dev 目录下 
 * 被 prepare_namespace 函数调用
 * prepare_namespace("/dev") */
int devtmpfs_mount(const char *mntdir)
{
	int err;

	if (!mount_dev)
		return 0;

	if (!dev_mnt)
		return 0;

	err = sys_mount("devtmpfs", (char *)mntdir, "devtmpfs", MS_SILENT, NULL);
	if (err)
		printk(KERN_INFO "devtmpfs: error mounting %i\n", err);
	else
		printk(KERN_INFO "devtmpfs: mounted\n");
	return err;
}

/*
 * Create devtmpfs instance, driver-core devices will add their device
 * nodes here.
 */
/* 创建 devtmpfs 实例，驱动核心的设备都将挂载到这里 */
int __init devtmpfs_init(void)
{
	int err;
	struct vfsmount *mnt;
	char options[] = "mode=0755";

    /* 向内核注册文件系统， 文件系统类型描述符为 dev_fs_type */
	err = register_filesystem(&dev_fs_type);
	if (err) {
		printk(KERN_ERR "devtmpfs: unable to register devtmpfs "
		       "type %i\n", err);
		return err;
	}

    /* 挂载 devtmpfs 文件系统，细节待分析 */
	mnt = kern_mount_data(&dev_fs_type, options);
	if (IS_ERR(mnt)) {
		err = PTR_ERR(mnt);
		printk(KERN_ERR "devtmpfs: unable to create devtmpfs %i\n", err);
		unregister_filesystem(&dev_fs_type);
		return err;
	}
	dev_mnt = mnt;

	printk(KERN_INFO "devtmpfs: initialized\n");
	return 0;
}
