#ifndef _tgl_osbuffer_h_
#define _tgl_osbuffer_h_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  void **zbs;
  void **framebuffers;
  int numbuffers;
  int xsize, ysize;
  int depth;
  int bytes_per_line;
} ostgl_context;

ostgl_context *
ostgl_create_context(const int xsize,
                     const int ysize,
                     const int depth,
                     void **framebuffers,
                     const int numbuffers);
void
ostgl_delete_context(ostgl_context *context);

void
ostgl_make_current(ostgl_context *context, const int index);

void
ostgl_resize(ostgl_context * context,
             const int xsize,
             const int ysize,
             void **framebuffers);

void
ostgl_swap_buffers(ostgl_context *context);

#ifdef __cplusplus
}
#endif

#endif /* _tgl_osbuffer_h_ */
