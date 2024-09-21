echo on
trace timestamp
trace resume_count_100
trace resume_count_200
trace resume_main_t3
trace resume_main_t4
trace resume_main_t5
trace resume_isr_t3
trace resume_isr_t4
trace resume_isr_t5
until pc 0 main
pc 0
run 1000
pc 0

memory timestamp
memory resume_count_100
memory resume_count_200
memory resume_main_t3
memory resume_main_t4
memory resume_main_t5
memory resume_isr_t3
memory resume_isr_t4
memory resume_isr_t5
