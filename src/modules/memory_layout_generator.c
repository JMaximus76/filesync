#include "memory_layout_generator.h"
#include <stdint.h>

void jfs_mlg_generate(jfs_mlg_layout_t *layout, size_t offsets[], const jfs_mlg_info_t *info, jfs_err_t *err) {
    size_t    total_bytes = info->header_size;
    size_t    master_align = info->header_align;

    VOID_FAIL_IF(total_bytes == 0, JFS_ERR_ARG);
    VOID_FAIL_IF((master_align - 1) & master_align, JFS_ERR_ARG);

    for (size_t i = 0; i < info->component_count; i++) {
        const jfs_mlg_component_t *component = &info->components[i];

        VOID_FAIL_IF((component->obj_align - 1) & component->obj_align, JFS_ERR_ARG);
        VOID_FAIL_IF(component->obj_count == 0, JFS_ERR_ARG);
        VOID_FAIL_IF(component->obj_size == 0, JFS_ERR_ARG);

        const size_t component_size = component->obj_size * component->obj_count;
        const size_t padding = (total_bytes + component->obj_align - 1) & ~(component->obj_align - 1);

        offsets[i] = total_bytes + padding;
        total_bytes += component_size + padding;
        if (master_align < component->obj_align) master_align = component->obj_align;
    }

    layout->total_bytes = total_bytes;
    layout->master_align = master_align;
}

void *jfs_mlg_allocate_block(jfs_mlg_layout_t *layout, jfs_err_t *err) {
    return jfs_aligned_alloc(layout->master_align, layout->total_bytes, err);
}

void *jfs_mlg_component_pointer(void *base_ptr, size_t offsets[], size_t component_index) {
    return ((uint8_t *) base_ptr) + offsets[component_index];
}
