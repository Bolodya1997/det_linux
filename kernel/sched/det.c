#include "sched.h"

static void update_curr_det(struct rq *rq);

void __init init_det_rq(struct det_rq *det_rq)
{
	INIT_LIST_HEAD(&det_rq->head);
	det_rq->next = NULL;
}

static inline
struct task_struct *task_of_det(struct sched_det_entity *det)
{
	return container_of(det, struct task_struct, det);
}

static inline
struct sched_det_entity *pick_next_det(struct det_rq *det_rq,
	struct sched_det_entity *det)
{
	if (!list_is_last(&det->list, &det_rq->head))
		return list_next_entry(det, list);

	return list_first_entry(&det_rq->head, struct sched_det_entity, list);
}

static
void enqueue_task_det(struct rq *rq, struct task_struct *p, int flags)
{
	struct det_rq *det_rq = &rq->det;
	struct sched_det_entity *det = &p->det;

	list_add_tail(&det->list, &det_rq->head);

	if (unlikely(!det_rq->next))
		det_rq->next = p;

	add_nr_running(rq, 1);
}

static
void dequeue_task_det(struct rq *rq, struct task_struct *p, int flags)
{
	struct det_rq *det_rq = &rq->det;
	struct sched_det_entity *det = &p->det;
	struct sched_det_entity *next_det;

	if (det_rq->next == p) {
		next_det = pick_next_det(det_rq, det);
		if (likely(next_det != det))
			det_rq->next = task_of_det(next_det);
		else
			det_rq->next = NULL;
	}

	list_del(&det->list);

	sub_nr_running(rq, 1);
}

static
void yield_task_det(struct rq *rq)
{
	struct det_rq *det_rq = &rq->det;
	struct sched_det_entity *det = &det_rq->next->det;
	struct sched_det_entity *next_det;

	next_det = pick_next_det(det_rq, det);
	det_rq->next = task_of_det(next_det);
}

static
void check_preempt_curr_det(struct rq *rq, struct task_struct *p, int flags)
{
}

static
struct task_struct *pick_next_task_det(struct rq *rq, struct task_struct *prev,
	struct rq_flags *rf)
{
	struct det_rq *det_rq = &rq->det;
	struct task_struct *next = det_rq->next;

	if (!next)
		return NULL;

	put_prev_task(rq, prev);

	next->se.exec_start = rq_clock_task(rq);

	return next;
}

static
void put_prev_task_det(struct rq *rq, struct task_struct *p)
{
	update_curr_det(rq);
}

#ifdef CONFIG_SMP
static
int select_task_rq_det(struct task_struct *p, int task_cpu, int sd_flag,
	int flags)
{
	return p->cpu;
}

static
void set_cpus_allowed_det(struct task_struct *p, const struct cpumask *newmask)
{
}

static
void rq_online_det(struct rq *rq)
{
}

static
void rq_offline_det(struct rq *rq)
{
}
#endif /* CONFIG_SMP */

static
void set_curr_task_det(struct rq *rq)
{
}

static
void task_tick_det(struct rq *rq, struct task_struct *p, int queued)
{
	update_curr_det(rq);
}

static
void switched_to_det(struct rq *this_rq, struct task_struct *task)
{
}

static
void prio_changed_det(struct rq *this_rq, struct task_struct *task,
	int oldprio)
{
}

static
void update_curr_det(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	u64 delta_exec;

	if (curr->sched_class != &det_sched_class)
		return;

	delta_exec = rq_clock_task(rq) - curr->se.exec_start;
	if (unlikely((s64) delta_exec <= 0))
		return;

	schedstat_set(curr->se.statistics.exec_max,
		max(curr->se.statistics.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq_clock_task(rq);
	cgroup_account_cputime(curr, delta_exec);
}

const struct sched_class det_sched_class = {
	.next = &fair_sched_class,

	.enqueue_task		= &enqueue_task_det,
	.dequeue_task		= &dequeue_task_det,
	.yield_task		= &yield_task_det,

	.check_preempt_curr	= &check_preempt_curr_det,	/* Empty */

	.pick_next_task		= &pick_next_task_det,
	.put_prev_task		= &put_prev_task_det,

#ifdef CONFIG_SMP
	.select_task_rq		= &select_task_rq_det,

	.set_cpus_allowed	= &set_cpus_allowed_det,	/* Empty */

	.rq_online		= &rq_online_det,		/* Empty */
	.rq_offline		= &rq_offline_det,		/* Empty */
#endif

	.set_curr_task		= &set_curr_task_det,		/* Empty */
	.task_tick		= &task_tick_det,

	.switched_to		= &switched_to_det,		/* Empty */
	.prio_changed		= &prio_changed_det,		/* Empty */

	.update_curr		= &update_curr_det
};
