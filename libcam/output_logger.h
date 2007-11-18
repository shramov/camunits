#ifndef __logger_unit_h__
#define __logger_unit_h__

#include <glib-object.h>

#include "unit.h"
#include "unit_driver.h"
#include "log.h"

G_BEGIN_DECLS

/**
 * CamLoggerUnit
 *
 */

typedef struct _CamLoggerUnit CamLoggerUnit;
typedef struct _CamLoggerUnitClass CamLoggerUnitClass;

// boilerplate
#define CAM_TYPE_LOGGER_UNIT  cam_logger_unit_get_type()
#define CAM_LOGGER_UNIT(obj)  (G_TYPE_CHECK_INSTANCE_CAST( (obj), \
        CAM_TYPE_LOGGER_UNIT, CamLoggerUnit))
#define CAM_LOGGER_UNIT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
            CAM_TYPE_LOGGER_UNIT, CamLoggerUnitClass ))
#define IS_CAM_LOGGER_UNIT(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
            CAM_TYPE_LOGGER_UNIT ))
#define IS_CAM_LOGGER_UNIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE( \
            (klass), CAM_TYPE_LOGGER_UNIT))
#define CAM_LOGGER_UNIT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
            CAM_TYPE_LOGGER_UNIT, CamLoggerUnitClass))

struct _CamLoggerUnit {
    CamUnit parent;
    CamUnitControl *record_ctl;
    CamUnitControl *filename_ctl;

    GAsyncQueue *msg_q;
    GThread *writer_thread;

    // as long as the writer thread is active, it "owns" these members
    CamLog *camlog;
    char *fname;
    char *basename;
};

struct _CamLoggerUnitClass {
    CamUnitClass parent_class;
};

GType cam_logger_unit_get_type (void);

/** 
 * Constructor.
 * 
 * Don't call this function manually.  Instead, use the unit driver
 */
CamLoggerUnit * cam_logger_unit_new(void);

CamUnitDriver * cam_logger_unit_driver_new(void);

G_END_DECLS

#endif
