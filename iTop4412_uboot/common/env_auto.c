#include <common.h>

#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <nand.h>
#include <movi.h>
#include <mmc.h>
#include <s5pc210.h>

/* references to names in env_common.c */
extern uchar default_environment[];
extern int default_environment_size;

char * env_name_spec = "SMDK bootable device";

env_t *env_ptr = 0;

/* local functions */
static void use_default(void);

DECLARE_GLOBAL_DATA_PTR;

uchar env_get_char_spec (int index)
{
	return ( *((uchar *)(gd->env_addr + index)) );
}

int env_init(void)
{
    /* 固定使用了 默认环境变量, 设置为有效标志 
     * 默认的环境变量并不一定是最终的，在后面的初始化过程中，有可能更改 */
	gd->env_addr  = (ulong)&default_environment[0];
	gd->env_valid = 1;

	return (0);
}

// TODO: 以下代码精简
/*
 * The legacy NAND code saved the environment in the first NAND device i.e.,
 * nand_dev_desc + 0. This is also the behaviour using the new NAND code.
 */
int saveenv_nand(void)
{
	return 0;
}

int saveenv_nand_adv(void)
{
	return 0;
}

/* 保存到 mmc 中， 
 * 还有个读出 env 的函数 void movi_read_env(ulong addr)
 * 这里需要测试下 */
int saveenv_movinand(void)
{
	mmc_init(find_mmc_device(0));
    movi_write_env(virt_to_phys((ulong)env_ptr));
    puts("done\n");

    return 1;
}

int saveenv_onenand(void)
{
    printf("OneNAND does not support the saveenv command\n");
    return 1;
}

/* env 保存动作，被 saveenv 命令调用 */
int saveenv(void)
{
    if (INF_REG3_REG == 1)
        saveenv_onenand();
    else if (INF_REG3_REG == 3)
        saveenv_movinand();
    else
        printf("Unknown boot device\n");

    return 0;
}

/*
 * The legacy NAND code saved the environment in the first NAND device i.e.,
 * nand_dev_desc + 0. This is also the behaviour using the new NAND code.
 */
void env_relocate_spec_nand(void)
{
	use_default();
}


void env_relocate_spec_movinand(void)
{
	uint *magic = (uint*)(PHYS_SDRAM_1);

	if ((0x24564236 != magic[0]) || (0x20764316 != magic[1])) {
	}
	
	if (crc32(0, env_ptr->data, ENV_SIZE) != env_ptr->crc)
		return use_default();
}

void env_relocate_spec_onenand(void)
{
}

void env_relocate_spec(void)
{
	if (INF_REG3_REG == 1)
		env_relocate_spec_onenand();
	else if (INF_REG3_REG == 2)
		env_relocate_spec_nand();
	else if (INF_REG3_REG == 3)
		env_relocate_spec_movinand();
	else
		use_default();
}

static void use_default()
{
	puts("*** Warning - using default environment\n\n");

	if (default_environment_size > CONFIG_ENV_SIZE) {
		puts("*** Error - default environment is too large\n\n");
		return;
	}

	memset (env_ptr, 0, sizeof(env_t));
	memcpy (env_ptr->data,
			default_environment,
			default_environment_size);
	env_ptr->crc = crc32(0, env_ptr->data, ENV_SIZE);
	gd->env_valid = 1;

}

