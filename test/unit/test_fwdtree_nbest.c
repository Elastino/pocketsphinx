#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "ngram_search_fwdtree.h"
#include "ngram_search_dag.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	pocketsphinx_t *ps;
	cmd_ln_t *config;
	acmod_t *acmod;
	ngram_search_t *ngs;
	clock_t c;
	int i;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, pocketsphinx_args(), TRUE,
				"-hmm", MODELDIR "/hmm/wsj1",
				"-lm", MODELDIR "/lm/swb/swb.lm.DMP",
				"-dict", MODELDIR "/lm/swb/swb.dic",
				"-fwdtree", "yes",
				"-fwdflat", "no",
				"-bestpath", "yes",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = pocketsphinx_init(config));

	ngs = (ngram_search_t *)ps->search;
	acmod = ps->acmod;
        acmod_set_grow(ps->acmod, TRUE);

	setbuf(stdout, NULL);
	c = clock();
	for (i = 0; i < 5; ++i) {
		FILE *rawfh;
		int16 buf[2048];
		size_t nread;
		int16 const *bptr;
		int nfr;
		ngram_dag_t *dag;
		ngram_nbest_t *nbest;
		latpath_t *path;
		int i;

		TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
		TEST_EQUAL(0, acmod_start_utt(acmod));
		ngram_fwdtree_start(ngs);
		while (!feof(rawfh)) {
			nread = fread(buf, sizeof(*buf), 2048, rawfh);
			bptr = buf;
			while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
				while (ngram_fwdtree_search(ngs)) {
				}
			}
		}
		ngram_fwdtree_finish(ngs);
		printf("FWDTREE: %s\n",
		       ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL)));

		TEST_ASSERT(acmod_end_utt(acmod) >= 0);
		fclose(rawfh);

		dag = ngram_dag_build(ngs);
		if (dag == NULL) {
			E_ERROR("Failed to build DAG!\n");
			return 1;
		}
		printf("BESTPATH: %s\n",
		       ngram_dag_hyp(dag, ngram_dag_bestpath(dag)));

		TEST_ASSERT(nbest = ngram_nbest_start(dag, 0, -1, -1, -1));
		i = 0;
		while ((path = ngram_nbest_next(nbest))) {
			if (i++ < 10)
				printf("NBEST %d: %s\n", i,
				       ngram_nbest_hyp(nbest, path));
		}
		ngram_nbest_finish(nbest);
	}
	TEST_EQUAL(0, strcmp("GO FOR WORDS TEN YEARS",
			     ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL))));
	c = clock() - c;
	printf("5 * fwdtree + bestpath + N-best search in %.2f sec\n",
	       (double)c / CLOCKS_PER_SEC);
	pocketsphinx_free(ps);

	return 0;
}