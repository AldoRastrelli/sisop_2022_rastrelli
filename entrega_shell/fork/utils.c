const int READ = 0;
const int WRITE = 1;

static int
process_is_parent(pid_t child_id)
{
	return child_id > 0;
}

static int
process_is_child(pid_t child_id)
{
	return child_id == 0;
}