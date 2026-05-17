/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM nv_char

#if !defined(_TRACE_NV_CHAR_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_NV_CHAR_H

#include <linux/tracepoint.h>
#include <linux/types.h>

TRACE_EVENT(nv_char_register,
	TP_PROTO(dev_t devt),
	TP_ARGS(devt),
	TP_STRUCT__entry(
		__field(dev_t, devt)
	),
	TP_fast_assign(
		__entry->devt = devt;
	),
	TP_printk("devt=%u:%u", MAJOR(__entry->devt), MINOR(__entry->devt))
);

TRACE_EVENT(nv_char_unregister,
	TP_PROTO(dev_t devt),
	TP_ARGS(devt),
	TP_STRUCT__entry(
		__field(dev_t, devt)
	),
	TP_fast_assign(
		__entry->devt = devt;
	),
	TP_printk("devt=%u:%u", MAJOR(__entry->devt), MINOR(__entry->devt))
);

TRACE_EVENT(nv_char_open,
	TP_PROTO(dev_t devt, int opens),
	TP_ARGS(devt, opens),
	TP_STRUCT__entry(
		__field(dev_t, devt)
		__field(int, opens)
	),
	TP_fast_assign(
		__entry->devt = devt;
		__entry->opens = opens;
	),
	TP_printk("devt=%u:%u opens=%d", MAJOR(__entry->devt),
		  MINOR(__entry->devt), __entry->opens)
);

TRACE_EVENT(nv_char_release,
	TP_PROTO(dev_t devt, int opens),
	TP_ARGS(devt, opens),
	TP_STRUCT__entry(
		__field(dev_t, devt)
		__field(int, opens)
	),
	TP_fast_assign(
		__entry->devt = devt;
		__entry->opens = opens;
	),
	TP_printk("devt=%u:%u opens=%d", MAJOR(__entry->devt),
		  MINOR(__entry->devt), __entry->opens)
);

#endif /* _TRACE_NV_CHAR_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH trace/events
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE nv_char

#include <trace/define_trace.h>
