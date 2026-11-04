suppressPackageStartupMessages({
  library(readxl); library(openxlsx); library(ggplot2); library(tidyr); library(dplyr); library(grid)
})
set.seed(12345)

BASE_DIR   <- "C:/Users/admin/Desktop/GenCrossover"   # folder with the xlsx; outputs are written here
REF        <- "NK=10"
PAIRS_MODE <- "all"
ALPHA      <- 0.05
B_BOOT     <- 10000
DPI        <- 300
if (!dir.exists(BASE_DIR)) BASE_DIR <- getwd()
IN_FILE    <- file.path(BASE_DIR, "tbl5.xlsx")
OUT_PREFIX <- file.path(BASE_DIR, "tbl5")

raw <- as.data.frame(read_excel(IN_FILE), check.names = FALSE)
names(raw)[1] <- "DATASET"
X <- as.matrix(raw[, -1]); rownames(X) <- raw$DATASET
N <- nrow(X); K <- ncol(X); methods <- colnames(X)
stopifnot(REF %in% methods)
cat(sprintf("Loaded %d datasets x %d methods\n", N, K))
stars <- function(p) ifelse(p < 0.001, "***", ifelse(p < 0.01, "**", ifelse(p < 0.05, "*", "ns")))
fp <- function(p) ifelse(p < 1e-4, formatC(p, format = "e", digits = 1), formatC(p, format = "f", digits = 4))

ranks <- t(apply(X, 1, rank, ties.method = "average")); colnames(ranks) <- methods
R <- colMeans(ranks)
desc <- data.frame(Method = methods, Mean = colMeans(X), SD = apply(X, 2, sd),
                   Median = apply(X, 2, median), MeanRank = R,
                   Best_or_tied = sapply(methods, function(m) sum(X[, m] <= apply(X, 1, min))),
                   row.names = NULL)

fr  <- friedman.test(X); chi <- unname(fr$statistic)
FF  <- (N - 1) * chi / (N * (K - 1) - chi)
p_ID <- pf(FF, K - 1, (K - 1) * (N - 1), lower.tail = FALSE)
W   <- chi / (N * (K - 1))
se  <- sqrt(K * (K + 1) / (6 * N))
CD  <- qtukey(1 - ALPHA, K, Inf) / sqrt(2) * se
nemenyi_p <- function(a, b) unname(ptukey(abs(R[a] - R[b]) / se * sqrt(2), K, Inf, lower.tail = FALSE))

nk_val <- as.numeric(sub("NK=", "", methods)); has_nk <- !anyNA(nk_val)
if (has_nk) {
  Rsum <- colSums(ranks)[order(nk_val, decreasing = TRUE)]
  Lp <- sum(seq_len(K) * Rsum)
  zL <- (Lp - N * K * (K + 1)^2 / 4) / sqrt(N * K^2 * (K^2 - 1) * (K + 1) / 144)
  p_page <- pnorm(zL, lower.tail = FALSE)
  rho <- apply(X, 1, function(r) suppressWarnings(cor(nk_val, r, method = "spearman")))
  n_neg <- sum(rho < 0, na.rm = TRUE); n_pos <- sum(rho > 0, na.rm = TRUE)
  p_sign <- binom.test(n_neg, n_neg + n_pos, 0.5)$p.value
}

paired_test <- function(a, b) {
  d <- b - a
  wt <- suppressWarnings(wilcox.test(b, a, paired = TRUE, exact = FALSE,
                                     conf.int = TRUE, conf.level = 1 - ALPHA))
  dn <- d[d != 0]; rk <- rank(abs(dn))
  z  <- qnorm(wt$p.value / 2, lower.tail = FALSE)
  data.frame(Wins_A = sum(d > 0), Ties = sum(d == 0), Wins_B = sum(d < 0),
             MeanDiff = mean(d), HL = unname(wt$estimate),
             HL_lo = wt$conf.int[1], HL_hi = wt$conf.int[2],
             RelImpr_pct = 100 * (mean(b) - mean(a)) / mean(b),
             r_effect = z / sqrt(length(dn)), p_Wilcoxon = wt$p.value)
}
pairs <- if (PAIRS_MODE == "ref") cbind(REF, setdiff(methods, REF)) else t(combn(methods, 2))
pw <- do.call(rbind, lapply(seq_len(nrow(pairs)), function(i) {
  A <- pairs[i, 1]; B <- pairs[i, 2]
  cbind(Comparison = paste(A, "vs", B), A = A, B = B, paired_test(X[, A], X[, B]),
        p_Nemenyi = nemenyi_p(A, B)) }))
pw$p_Holm <- p.adjust(pw$p_Wilcoxon, "holm")
pw$Signif <- stars(pw$p_Holm)
pw$Comparison <- factor(pw$Comparison, levels = pw$Comparison)

wb <- createWorkbook(); addWorksheet(wb, "Statistics"); sh <- "Statistics"
hs  <- createStyle(textDecoration = "bold", fgFill = "#DCE6F1", border = "bottom", halign = "center")
ts  <- createStyle(textDecoration = "bold", fontSize = 12, fontColour = "#1F3864")
pst <- createStyle(numFmt = "0.00E+00"); nst <- createStyle(numFmt = "0.00")
row <- 1
put_title <- function(txt) { writeData(wb, sh, txt, startRow = row, startCol = 1); addStyle(wb, sh, ts, row, 1); row <<- row + 1 }
put_df <- function(df, pcols = character()) {
  writeData(wb, sh, df, startRow = row, startCol = 1, headerStyle = hs)
  for (pc in pcols) addStyle(wb, sh, pst, rows = row + seq_len(nrow(df)), cols = which(names(df) == pc), gridExpand = TRUE)
  row <<- row + nrow(df) + 2
}
put_title(paste0("tbl5 - ", "Table 5: number of chromosomes (NK) in the crossover"))
put_df(data.frame(Item = c("Datasets (N)", "Methods (k)", "Reference", "Metric", "Sign convention", "Alpha"),
                  Value = c(N, K, REF, "Classification error % (lower = better)",
                            "Pair 'A vs B': difference = error(B) - error(A); positive => A better", ALPHA)))
put_title("1. Omnibus tests (all methods)")
omni <- data.frame(Test = c("Friedman chi-square", "Iman-Davenport F", "Kendall W (effect size)",
                            "Nemenyi critical difference (mean ranks)"),
                   Statistic = c(chi, FF, W, CD),
                   df = c(K - 1, paste0(K - 1, ", ", (K - 1) * (N - 1)), NA, NA),
                   p_value = c(fr$p.value, p_ID, NA, NA))
if (has_nk) omni <- rbind(omni,
  data.frame(Test = c("Page trend test (error decreases with NK, one-sided)",
                      "Per-dataset Spearman(NK, error): median rho; sign test"),
             Statistic = c(zL, median(rho, na.rm = TRUE)), df = c(NA, sprintf("%d neg / %d pos", n_neg, n_pos)),
             p_value = c(p_page, p_sign)))
put_df(omni, "p_value")
put_title("2. Descriptive statistics (sorted by mean rank, 1 = best)")
d_out <- desc[order(desc$MeanRank), ]; d_out[, 2:5] <- round(d_out[, 2:5], 3)
put_df(d_out)
put_title("3. Pairwise paired comparisons (Wilcoxon signed-rank, Holm-adjusted)")
x_out <- data.frame(Comparison = as.character(pw$Comparison),
                    `Wins A` = pw$Wins_A, Ties = pw$Ties, `Wins B` = pw$Wins_B,
                    MeanDiff = round(pw$MeanDiff, 3), HL_median_diff = round(pw$HL, 3),
                    `HL 95% CI` = sprintf("[%.2f, %.2f]", pw$HL_lo, pw$HL_hi),
                    `Rel. improvement A %` = round(pw$RelImpr_pct, 2),
                    r_effect = round(pw$r_effect, 3), p_Wilcoxon = pw$p_Wilcoxon,
                    p_Holm = pw$p_Holm, Signif = pw$Signif, p_Nemenyi = pw$p_Nemenyi,
                    check.names = FALSE)
put_df(x_out, c("p_Wilcoxon", "p_Holm", "p_Nemenyi"))
writeData(wb, sh, "Signif (Holm): *** p<0.001, ** p<0.01, * p<0.05, ns = not significant. r_effect: |r|~0.1 small, 0.3 medium, 0.5 large.",
          startRow = row, startCol = 1)
setColWidths(wb, sh, cols = 1:13, widths = c(30, 12, 12, 12, 12, 16, 18, 20, 11, 13, 13, 9, 13))
saveWorkbook(wb, paste0(OUT_PREFIX, "_statistics.xlsx"), overwrite = TRUE)

## ======================= FIGURE (print-ready) ===============================
## Designed at its final print size: FIG_W inches wide. Insert it in LyX with
## width = 100% of the text width (not with "scale"), so the fonts print at ~8-10 pt.
FIG_W <- 6.5; FIG_H <- 8.2; BASE <- 10; ANN <- 3.1      # ANN = annotation size in mm (3.1 mm ~ 9 pt)
has_nk <- all(grepl("^NK=[0-9]+$", methods))
mlab <- function(m) if (has_nk) parse(text = paste0("N[K]==", sub("NK=", "", m))) else m
## pair labels: "vs B" when every pair starts with REF, otherwise "A\nvs B"
plab <- function(cmp) {
  pp <- strsplit(as.character(cmp), " vs ")
  if (has_nk) return(parse(text = vapply(pp, function(p) paste0("N[K]==", sub("NK=", "", p[1]),
                                                                 "~'vs'~N[K]==", sub("NK=", "", p[2])), "")))
  if (PAIRS_MODE == "ref") return(vapply(pp, function(p) paste("vs", p[2]), ""))
  vapply(pp, function(p) paste0(p[1], "\nvs ", p[2]), "")
}
theme_set(theme_bw(base_size = BASE) +
          theme(panel.grid.minor = element_blank(),
                plot.title = element_text(face = "bold", size = BASE + 1, margin = margin(b = 4)),
                axis.title = element_text(size = BASE),
                axis.text  = element_text(size = BASE - 1, colour = "black"),
                legend.text = element_text(size = BASE - 1),
                plot.title.position = "plot",
                plot.margin = margin(4, 6, 4, 4)))
col_ref <- "#C0392B"; col_oth <- "#5D8AA8"
ord  <- methods[order(R)]
cols <- setNames(ifelse(methods == REF, col_ref, col_oth), methods)
many <- K > 5 || max(nchar(methods)) > 9        # rotate method names when there are many or long ones
long <- as.data.frame(X) |> mutate(DATASET = rownames(X)) |>
  pivot_longer(-DATASET, names_to = "Method", values_to = "Error") |>
  mutate(Method = factor(Method, levels = methods))

## (top left) box plots
pa <- ggplot(long, aes(Method, Error)) +
  geom_line(aes(group = DATASET), colour = "grey80", linewidth = .25) +
  geom_boxplot(aes(fill = Method), outlier.shape = NA, alpha = .75, width = .6, linewidth = .4) +
  geom_point(size = .7, alpha = .5) +
  stat_summary(fun = mean, geom = "point", shape = 23, size = 2.4, fill = "white") +
  scale_fill_manual(values = cols, guide = "none") + scale_x_discrete(labels = mlab(levels(long$Method))) +
  labs(x = NULL, y = "Classification error (%)", title = "Error per method") +
  theme(axis.text.x = if (many) element_text(angle = if (K > 5) 45 else 25, hjust = 1, vjust = 1) else element_text())

## (top right) paired comparisons: Hodges-Lehmann estimate and 95% CI
fdat <- pw |> mutate(Comparison = factor(Comparison, levels = rev(levels(Comparison))),
                     lab = paste0(Signif, "  ", Wins_A, "/", Ties, "/", Wins_B))
pb <- ggplot(fdat, aes(HL, Comparison)) +
  geom_vline(xintercept = 0, linetype = 2, colour = "grey40") +
  geom_errorbar(aes(xmin = HL_lo, xmax = HL_hi), width = .3, orientation = "y", linewidth = .5) +
  geom_point(size = 2.2, colour = col_ref) +
  geom_label(aes(x = HL_hi, label = lab), hjust = -0.08, size = ANN, fill = "white", label.size = 0,
             label.padding = unit(0.08, "lines")) +
  scale_x_continuous(expand = expansion(mult = c(.05, .55))) +
  scale_y_discrete(labels = plab(levels(fdat$Comparison))) +
  labs(x = "error(B) - error(A), 95% CI", y = NULL,
       title = if (PAIRS_MODE == "ref") paste(REF, "vs others") else "Paired comparisons")

## (bottom left) mean ranks +/- CD/2
rk <- data.frame(Method = factor(ord, levels = rev(ord)), R = R[ord])
pc <- ggplot(rk, aes(R, Method)) +
  geom_errorbar(aes(xmin = R - CD / 2, xmax = R + CD / 2), width = .3, orientation = "y", colour = "grey30", linewidth = .5) +
  geom_point(aes(colour = Method == REF), size = 2.4) +
  geom_text(aes(label = sprintf("%.2f", R)), vjust = -0.9, size = ANN) +
  scale_colour_manual(values = c("TRUE" = col_ref, "FALSE" = col_oth), guide = "none") +
  scale_y_discrete(labels = mlab(levels(rk$Method)), expand = expansion(add = c(.6, .9))) +
  labs(x = "Mean rank (1 = lowest error)", y = NULL, title = sprintf("Mean ranks (CD = %.2f)", CD))

## (bottom right) per-dataset differences, or the N_K trend
dl <- do.call(rbind, lapply(seq_len(nrow(pairs)), function(i)
  data.frame(DATASET = rownames(X), Comparison = paste(pairs[i, 1], "vs", pairs[i, 2]),
             Diff = X[, pairs[i, 2]] - X[, pairs[i, 1]]))) |>
  mutate(Comparison = factor(Comparison, levels = as.character(pw$Comparison)))
dl$Comparison <- factor(dl$Comparison, levels = rev(levels(dl$Comparison)))     # first pair on top
lab_df <- data.frame(Comparison = factor(as.character(pw$Comparison), levels = levels(dl$Comparison)),
                     lab = pw$Signif, x = tapply(dl$Diff, dl$Comparison, max)[as.character(pw$Comparison)])
pd <- ggplot(dl, aes(Diff, Comparison)) +
  geom_vline(xintercept = 0, linetype = 2) +
  geom_violin(fill = "grey92", colour = "grey60", scale = "width", linewidth = .3, orientation = "y") +
  geom_boxplot(width = .16, outlier.shape = NA, fill = "white", linewidth = .35, orientation = "y") +
  geom_jitter(aes(colour = Diff > 0), height = .08, width = 0, size = .9) +
  geom_text(data = lab_df, aes(x = x, label = lab), hjust = -0.45, fontface = "bold", size = ANN) +
  scale_colour_manual(values = c("TRUE" = "#2E86C1", "FALSE" = col_ref), breaks = c("TRUE", "FALSE"),
                      labels = c("TRUE" = "A better", "FALSE" = "B better"), name = NULL) +
  scale_x_continuous(expand = expansion(mult = c(.05, .15))) +
  scale_y_discrete(labels = plab(levels(dl$Comparison))) +
  labs(x = "error(B) - error(A) (pp)", y = NULL, title = "Differences per dataset") +
  theme(legend.position = "bottom", legend.margin = margin(0, 0, 0, 0))
if (has_nk && exists("zL")) {
  nk_val <- as.numeric(sub("NK=", "", methods))
  tr <- do.call(rbind, lapply(seq_along(methods), function(i) {
    idx <- replicate(B_BOOT, sample.int(N, replace = TRUE))
    dd  <- X[, i] - X[, 1]
    bm  <- apply(idx, 2, function(ix) mean(dd[ix])); br <- apply(idx, 2, function(ix) mean(ranks[ix, i]))
    rbind(data.frame(NK = nk_val[i], Metric = paste0("Error~change~vs~N[K]==", nk_val[1], "~(pp)"),
                     Value = mean(dd), lo = quantile(bm, ALPHA / 2), hi = quantile(bm, 1 - ALPHA / 2)),
          data.frame(NK = nk_val[i], Metric = "Mean~rank", Value = mean(ranks[, i]),
                     lo = quantile(br, ALPHA / 2), hi = quantile(br, 1 - ALPHA / 2)))
  }))
  tr$Metric <- factor(tr$Metric, levels = unique(tr$Metric))
  pd <- ggplot(tr, aes(NK, Value)) +
    geom_ribbon(aes(ymin = lo, ymax = hi), alpha = .25, fill = col_oth) +
    geom_line(colour = col_oth, linewidth = .7) + geom_point(size = 2, colour = col_ref) +
    geom_hline(data = data.frame(Metric = factor(levels(tr$Metric)[1], levels = levels(tr$Metric)), y = 0),
               aes(yintercept = y), linetype = 2, colour = "grey40") +
    scale_x_continuous(breaks = nk_val) +
    facet_wrap(~Metric, scales = "free_y", ncol = 1, labeller = label_parsed) +
    labs(x = expression(N[K]), y = NULL, title = expression(bold("Effect of ") * bold(N[K]))) +
    theme(strip.text = element_text(size = BASE - 1, margin = margin(2, 0, 2, 0)))
}

## header (omnibus tests)
txt  <- sprintf("Friedman chi2(%d) = %.2f, p = %s  |  Iman-Davenport F = %.2f, p = %s  |  Kendall W = %.2f",
                K - 1, chi, fp(fr$p.value), FF, fp(p_ID), W)
txt2 <- NULL
if (has_nk && exists("zL"))
  txt2 <- parse(text = sprintf("bold(paste('Page trend: z = %.2f, p = %s  |  Spearman(', N[K], ', error) < 0 in %d/%d datasets (sign test p = %s)'))",
                               zL, fp(p_page), n_neg, n_neg + n_pos, fp(p_sign)))

draw_fig <- function() {
  grid.newpage()
  hh <- if (is.null(txt2)) 0.035 else 0.065                       # header height (fraction)
  gp_t <- gpar(fontface = "bold", fontsize = BASE - 0.5, col = "#1F3864")
  grid.text(txt, x = 0.01, y = 1 - 0.018, just = c("left", "center"), gp = gp_t)
  if (!is.null(txt2)) grid.text(txt2, x = 0.01, y = 1 - 0.048, just = c("left", "center"), gp = gp_t)
  top <- 1 - hh; h1 <- top * 0.5; h2 <- top - h1
  place <- function(p, x, y, w, h) print(p, vp = viewport(x = x, y = y, width = w, height = h, just = c("left", "bottom")))
  place(pa, 0,   h2, .48, h1); place(pb, .48, h2, .52, h1)
  place(pc, 0,   0,  .48, h2); place(pd, .48, 0,  .52, h2)
}
png(paste0(OUT_PREFIX, "_summary.png"), width = FIG_W, height = FIG_H, units = "in", res = DPI)
draw_fig(); dev.off()
pdf(paste0(OUT_PREFIX, "_summary.pdf"), width = FIG_W, height = FIG_H, useDingbats = FALSE)   # vector version for print
draw_fig(); dev.off()

cat("\n", txt, "\n"); print(data.frame(pw[, c("Comparison", "Wins_A", "Ties", "Wins_B", "MeanDiff", "p_Wilcoxon", "p_Holm", "r_effect", "Signif")]), row.names = FALSE)
if (has_nk && exists("zL")) cat(sprintf("Page trend: z = %.3f, p = %s ; Spearman sign test p = %s\n", zL, fp(p_page), fp(p_sign)))
cat("\nDone. Files written with prefix:", OUT_PREFIX, "\n")
