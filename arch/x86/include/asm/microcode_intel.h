#ifndef _ASM_X86_MICROCODE_INTEL_H
#define _ASM_X86_MICROCODE_INTEL_H

#include <asm/microcode.h>

struct microcode_header_intel {
	unsigned int            hdrver;
	unsigned int            rev;
	unsigned int            date;
	unsigned int            sig;
	unsigned int            cksum;
	unsigned int            ldrver;
	unsigned int            pf;
	unsigned int            datasize;
	unsigned int            totalsize;
	unsigned int            reserved[3];
};

struct microcode_intel {
	struct microcode_header_intel hdr;
	unsigned int            bits[0];
};

/* microcode format is extended from prescott processors */
struct extended_signature {
	unsigned int            sig;
	unsigned int            pf;
	unsigned int            cksum;
};

struct extended_sigtable {
	unsigned int            count;
	unsigned int            cksum;
	unsigned int            reserved[3];
	struct extended_signature sigs[0];
};

#define DEFAULT_UCODE_DATASIZE	(2000)
#define MC_HEADER_SIZE		(sizeof(struct microcode_header_intel))
#define DEFAULT_UCODE_TOTALSIZE (DEFAULT_UCODE_DATASIZE + MC_HEADER_SIZE)
#define EXT_HEADER_SIZE		(sizeof(struct extended_sigtable))
#define EXT_SIGNATURE_SIZE	(sizeof(struct extended_signature))
#define DWSIZE			(sizeof(u32))

#define get_totalsize(mc) \
	(((struct microcode_intel *)mc)->hdr.totalsize ? \
	 ((struct microcode_intel *)mc)->hdr.totalsize : \
	 DEFAULT_UCODE_TOTALSIZE)

#define get_datasize(mc) \
	(((struct microcode_intel *)mc)->hdr.datasize ? \
	 ((struct microcode_intel *)mc)->hdr.datasize : DEFAULT_UCODE_DATASIZE)

#define sigmatch(s1, s2, p1, p2) \
	(((s1) == (s2)) && (((p1) & (p2)) || (((p1) == 0) && ((p2) == 0))))

#define exttable_size(et) ((et)->count * EXT_SIGNATURE_SIZE + EXT_HEADER_SIZE)

extern int
get_matching_microcode(unsigned int csig, int cpf, void *mc, int rev);
extern int microcode_sanity_check(void *mc, int print_err);
extern int get_matching_sig(unsigned int csig, int cpf, void *mc, int rev);
extern int
update_match_revision(struct microcode_header_intel *mc_header, int rev);

#ifdef CONFIG_MICROCODE_INTEL_EARLY
extern enum ucode_state
get_matching_model_microcode(int cpu, void *data, size_t size,
			     struct mc_saved_data *mc_saved_data,
			     struct microcode_intel **mc_saved_in_initrd,
			     enum system_states system_state);
extern enum ucode_state
generic_load_microcode_early(int cpu, struct microcode_intel **mc_saved_p,
			     unsigned int mc_saved_count,
			     struct ucode_cpu_info *uci);
extern void __init
load_ucode_intel_bsp(char *real_mode_data);
extern void __init load_ucode_intel_ap(void);
#else
static inline enum ucode_state
get_matching_model_microcode(int cpu, void *data, size_t size,
			     struct mc_saved_data *mc_saved_data,
			     struct microcode_intel **mc_saved_in_initrd,
			     enum system_states system_state)
{
	return UCODE_ERROR;
}
static inline enum ucode_state
generic_load_microcode_early(int cpu, struct microcode_intel **mc_saved_p,
			     unsigned int mc_saved_count,
			     struct ucode_cpu_info *uci)
{
	return UCODE_ERROR;
}
static inline __init void
load_ucode_intel_bsp(char *real_mode_data)
{
}
static inline __init void
load_ucode_intel_ap(struct ucode_cpu_info *uci,
		    struct mc_saved_data *mc_saved_data)
{
}
#endif

#endif /* _ASM_X86_MICROCODE_INTEL_H */
