#include "dmg.h"
#include "log.h"
#include "profiler.h"
#include "ui.h"

static int
run_dmg(void* data) {
  profiler_set_thread_name("dmg");
  dmg_run((struct dmg_system*)data);
  return 0;
}

int
main(void) {
  profiler_start();
  profiler_set_thread_name("ui");
  int rc = ui_start();
  if (rc != 0) {
    return rc;
  }

  struct dmg_system dmg;
  dmg_init(&dmg);
  rc = ui_run(run_dmg, &dmg);
  dmg_destroy(&dmg);

  ui_stop();
  return rc;
}
