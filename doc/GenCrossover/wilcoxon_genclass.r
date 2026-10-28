#!/usr/bin/env Rscript
# Reproducible paired Wilcoxon signed-rank tests from newAverages.
# Exact conditional sign-permutation distribution (including tied absolute differences).
# Requires: install.packages('readODS')

args <- commandArgs(trailingOnly = TRUE)
file <- if (length(args) >= 1) args[1] else 'new_genclass(2).ods'
if (!requireNamespace('readODS', quietly = TRUE)) {
  stop("Install the readODS package: install.packages('readODS')")
}
raw <- as.data.frame(readODS::read_ods(file, sheet = 'newAverages', col_names = TRUE),
                     check.names = FALSE)
names(raw)[1] <- 'DATASET'
required <- c('DATASET', 'CLASS', 'standard', 'targeted', 'targetedWorst')
if (!all(required %in% names(raw))) {
  stop('Missing columns: ', paste(setdiff(required, names(raw)), collapse = ', '))
}
raw$DATASET <- trimws(as.character(raw$DATASET))
raw <- raw[!is.na(raw$DATASET) & nzchar(raw$DATASET) &
           toupper(raw$DATASET) != 'AVERAGE', , drop = FALSE]
for (nm in required[-1]) raw[[nm]] <- as.numeric(raw[[nm]])
if (anyNA(raw[, required[-1]])) stop('Missing/non-numeric classification errors.')
if (anyDuplicated(raw$DATASET)) stop('Duplicate dataset names.')

eeg <- c('Z_F_S', 'Z_O_N_F_S', 'ZO_NF_S', 'ZONF_S')
if (!all(eeg %in% raw$DATASET)) stop('One or more EEG datasets were not found.')

# Exact two-sided Wilcoxon signed-rank test by subset-sum dynamic programming.
# Zero differences are removed (Wilcoxon convention); tied |differences| receive
# average ranks. Each sign assignment is equally likely under the null.
exact_signed_rank <- function(diff) {
  diff <- diff[diff != 0]
  n <- length(diff)
  if (!n) return(1)
  ranks2 <- as.integer(round(2 * rank(abs(diff), ties.method = 'average')))
  observed <- sum(ranks2[diff > 0])
  maxsum <- sum(ranks2)
  counts <- numeric(maxsum + 1L)
  counts[1L] <- 1
  reachable <- 0L
  for (r in ranks2) {
    for (s in seq.int(reachable, 0L, by = -1L)) {
      counts[s + r + 1L] <- counts[s + r + 1L] + counts[s + 1L]
    }
    reachable <- reachable + r
  }
  lower <- sum(counts[seq_len(observed + 1L)]) / 2^n
  upper <- sum(counts[(observed + 1L):(maxsum + 1L)]) / 2^n
  min(1, 2 * min(lower, upper))
}

methods <- c(STANDARD = 'standard', TARGETED = 'targeted',
             TARGETEDWORST = 'targetedWorst')
run_tests <- function(dat, label) {
  result <- do.call(rbind, lapply(names(methods), function(m) {
    d <- dat$CLASS - dat[[methods[[m]]]] # positive = crossover has lower error
    data.frame(analysis = label, method = m, datasets = nrow(dat),
               wins = sum(d > 0), losses = sum(d < 0), ties = sum(d == 0),
               p_exact = exact_signed_rank(d))
  }))
  result$p_holm <- p.adjust(result$p_exact, method = 'holm')
  result
}

results <- rbind(run_tests(raw, 'All datasets'),
                 run_tests(raw[!raw$DATASET %in% eeg, , drop = FALSE],
                           'Excluding EEG'))
print(results, row.names = FALSE, digits = 10)
write.csv(results, 'wilcoxon_genclass_results.csv', row.names = FALSE)
cat('\nSaved: wilcoxon_genclass_results.csv\n')
cat('Note: tests use per-dataset means, not 30 independent run results.\n')
