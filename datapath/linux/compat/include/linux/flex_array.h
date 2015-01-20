#ifndef __LINUX_FLEX_ARRAY_WRAPPER_H
#define __LINUX_FLEX_ARRAY_WRAPPER_H

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#include_next <linux/flex_array.h>
#else

#include <linux/reciprocal_div.h>
#include <linux/types.h>
#include <asm/page.h>

#define FLEX_ARRAY_PART_SIZE PAGE_SIZE
#define FLEX_ARRAY_BASE_SIZE PAGE_SIZE

struct flex_array_part;

/*
 * This is meant to replace cases where an array-like
 * structure has gotten too big to fit into kmalloc()
 * and the developer is getting tempted to use
 * vmalloc().
 */

// 哈希桶结构
// 一个桶中存放的是 hash 值碰撞的元素
// 因此，仅根据 hash 值在 flex_array 结构中找到对应的 bucket 后，还需要遍历该 bucket 中的所有元素以确定
struct flex_array {
	union {
		struct {
			int element_size;      // 下面弹性数组中元素大小
			int total_nr_elements; // 下面弹性数组元素总个数
			int elems_per_part;    // 每个 part 指针指向的空间能存储多少元素
			struct reciprocal_value reciprocal_elems;
			// 弹性数组，数组的可用大小为: PAGE_SIZE - 前面所有成员大小
			struct flex_array_part *parts[];
		};
		/*
		 * This little trick makes sure that
		 * sizeof(flex_array) == PAGE_SIZE
		 */
		// padding 是占位符，保证 flex_array 的 union 结构体大小为 PAGE_SIZE(4k) 这么大。
		// PS: union 的大小取决于其最大的成员的大小。
		char padding[FLEX_ARRAY_BASE_SIZE];
	};
};

/* Number of bytes left in base struct flex_array, excluding metadata */
#define FLEX_ARRAY_BASE_BYTES_LEFT					\
	(FLEX_ARRAY_BASE_SIZE - offsetof(struct flex_array, parts))

/* Number of pointers in base to struct flex_array_part pages */
#define FLEX_ARRAY_NR_BASE_PTRS						\
	(FLEX_ARRAY_BASE_BYTES_LEFT / sizeof(struct flex_array_part *))

/* Number of elements of size that fit in struct flex_array_part */
#define FLEX_ARRAY_ELEMENTS_PER_PART(size)				\
	(FLEX_ARRAY_PART_SIZE / size)

/*
 * Defines a statically allocated flex array and ensures its parameters are
 * valid.
 */
#define DEFINE_FLEX_ARRAY(__arrayname, __element_size, __total)		\
	struct flex_array __arrayname = { { {				\
		.element_size = (__element_size),			\
		.total_nr_elements = (__total),				\
	} } };								\
	static inline void __arrayname##_invalid_parameter(void)	\
	{								\
		BUILD_BUG_ON((__total) > FLEX_ARRAY_NR_BASE_PTRS *	\
			FLEX_ARRAY_ELEMENTS_PER_PART(__element_size));	\
	}

struct flex_array *flex_array_alloc(int element_size, unsigned int total,
		gfp_t flags);
int flex_array_prealloc(struct flex_array *fa, unsigned int start,
		unsigned int nr_elements, gfp_t flags);
void flex_array_free(struct flex_array *fa);
void flex_array_free_parts(struct flex_array *fa);
int flex_array_put(struct flex_array *fa, unsigned int element_nr, void *src,
		gfp_t flags);
int flex_array_clear(struct flex_array *fa, unsigned int element_nr);
void *flex_array_get(struct flex_array *fa, unsigned int element_nr);
int flex_array_shrink(struct flex_array *fa);

#define flex_array_put_ptr(fa, nr, src, gfp) \
	flex_array_put(fa, nr, (void *)&(src), gfp)

void *flex_array_get_ptr(struct flex_array *fa, unsigned int element_nr);

#endif /* Linux version < 3.0.0 */
#endif /* __LINUX_FLEX_ARRAY_WRAPPER_H */
