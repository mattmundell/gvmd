/* Test 4 of getting a report.
 * $Id$
 * Description: Test GET_REPORT with extra elements.
 *
 * Authors:
 * Matthew Mundell <matt@mundell.ukfsn.org>
 *
 * Copyright:
 * Copyright (C) 2009 Greenbone Networks GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * or, at your option, any later version as published by the Free
 * Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define TRACE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "../tracef.h"
#include "../string.h"

int
main ()
{
  int socket;
  gnutls_session_t session;
  char* id;

  setup_test ();

  socket = connect_to_manager (&session);
  if (socket == -1) return EXIT_FAILURE;

  /* Create a task. */

  if (env_authenticate (&session)) goto fail;

  if (create_task_from_rc_file (&session,
                                "new_task_small_rc",
                                "Task for omp_get_report_0",
                                "Test omp_get_report_0 task.",
                                &id))
    goto fail;

  /* Start the task. */

  if (start_task (&session, id)) goto delete_fail;

  /* Wait for the task to finish on the server. */

  if (wait_for_task_end (&session, id)) goto delete_fail;

  /* Request the status. */

  if (openvas_server_sendf (&session,
                            "<get_status task_id=\"%s\"/>",
                            id)
      == -1)
    goto delete_fail;

  /* Read the first report ID from the response. */

  entity_t entity = NULL;
  if (read_entity (&session, &entity))
    {
      fprintf (stderr, "Failed to read response.\n");
      goto delete_fail;
    }
  entity_t task = entity_child (entity, "task");
  if (task == NULL)
    {
      fprintf (stderr, "Failed to find task.\n");
      goto free_fail;
    }
  entity_t report = entity_child (task, "report");
  if (report == NULL)
    {
      fprintf (stderr, "Failed to find report.\n");
      goto free_fail;
    }
  const char* report_id = entity_attribute (report, "id");
  if (report_id == NULL)
    {
      fprintf (stderr, "Failed to find report id.\n");
      goto free_fail;
    }

  /* Get the report. */

  if (openvas_server_sendf (&session,
                            "<get_report format=\"nbe\" report_id=\"%s\">"
                            "<error/>"
                            "</get_report>",
                            report_id)
      == -1)
    goto delete_fail;

  /* Read the response. */

  entity = NULL;
  if (read_entity (&session, &entity))
    {
      fprintf (stderr, "Failed to read response.\n");
      goto delete_fail;
    }
  if (entity) print_entity (stdout, entity);

  /* Compare to expected response. */

  if (entity
      && (strcmp (entity_name (entity), "get_report_response") == 0)
      && entity_attribute (entity, "status")
      && (strcmp (entity_attribute (entity, "status"), "400") == 0))
    {
      free_entity (entity);
      delete_task (&session, id);
      free (id);
      close_manager_connection (socket, session);
      return EXIT_SUCCESS;
    }

 free_fail:
  free_entity (entity);
 delete_fail:
  delete_task (&session, id);
  free (id);
 fail:
  close_manager_connection (socket, session);
  return EXIT_FAILURE;
}
