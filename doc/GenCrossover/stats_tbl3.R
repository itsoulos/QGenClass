suppressPackageStartupMessages({
  library(readxl); library(openxlsx); library(ggplot2); library(tidyr); library(dplyr); library(grid)
})
set.seed(12345)

BASE_DIR   <- "C:/Users/admin/Desktop/GenCrossover"
REF        <- "STANDARD"
PAIRS_MODE <- "ref"
ALPHA      <- 0.05
B_BOOT     <- 10000
DPI        <- 300
if (!dir.exists(BASE_DIR)) BASE_DIR <- getwd()
IN_FILE    <- file.path(BASE_DIR, "tbl3.xlsx")
OUT_PREFIX <- file.path(BASE_DIR, "tbl3")

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
put_title(paste0("tbl3 - ", "Table 3: proposed method (STANDARD) vs other methods"))
put_df(data.frame(Item = c("Datasets (N)", "Methods (k)", "Reference", "Metric", "Sign convention", "Alpha"),
                  Value = c(N, K, REF, "Classification error % (lower = better)",
                            "Pair 'A vs B': difference = error(B) - error(A); positive => A better", ALPHA)))
put_title("1. Omnibus tests (all methods)")
omni <- data.frame(Test = c("Friedman chi-square", "Iman-Davenport F", "Kendall W (effect size)",
                            "Nemenyi critical difference (mean ranks)"),
                   Statistic = c(chi, FF, W, CD),
                   df = c(K - 1, paste0(K - 1, ", ", (K - 1) * (N - 1)), NA, NA),
                   p_value = c(fr$p.value, p_ID, NA, NA))
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

has_nk <- all(grepl("^NK=[0-9]+$", methods))
mlab <- function(m) if (has_nk) parse(text = paste0("N[K]==", sub("NK=", "", m))) else m
plab <- function(cmp) if (has_nk) parse(text = vapply(strsplit(as.character(cmp), " vs "), function(p)
  paste0("N[K]==", sub("NK=", "", p[1]), "~'vs'~N[K]==", sub("NK=", "", p[2])), "")) else as.character(cmp)
theme_set(theme_bw(base_size = 10) + theme(panel.grid.minor = element_blank(),
                                           plot.title = element_text(face = "bold", size = 10),
                                           plot.tag = element_text(face = "bold")))
col_ref <- "#C0392B"; col_oth <- "#5D8AA8"
ord <- methods[order(R)]
cols <- setNames(ifelse(methods == REF, col_ref, col_oth), methods)
place <- function(p, x, y, w, h) print(p, vp = viewport(x = x, y = y, width = w, height = h, just = c("left", "bottom")))
long <- as.data.frame(X) |> mutate(DATASET = rownames(X)) |>
  pivot_longer(-DATASET, names_to = "Method", values_to = "Error") |>
  mutate(Method = factor(Method, levels = methods))

pa <- ggplot(long, aes(Method, Error)) +
  geom_line(aes(group = DATASET), colour = "grey75", linewidth = .25) +
  geom_boxplot(aes(fill = Method), outlier.shape = NA, alpha = .7, width = .5) +
  geom_point(size = .8, alpha = .5) +
  stat_summary(fun = mean, geom = "point", shape = 23, size = 2.8, fill = "white") +
  scale_fill_manual(values = cols, guide = "none") + scale_x_discrete(labels = mlab(levels(long$Method))) +
  labs(x = NULL, y = "Classification error (%)",
       title = "Error distribution per method", subtitle = "grey lines = same dataset; diamond = mean")

fdat <- pw |> mutate(Comparison = factor(Comparison, levels = rev(levels(Comparison))),
                     lab = paste0("p=", fp(p_Holm), " ", Signif, "  (", Wins_A, "/", Ties, "/", Wins_B, ")"))
pb <- ggplot(fdat, aes(HL, Comparison)) +
  geom_vline(xintercept = 0, linetype = 2, colour = "grey40") +
  geom_errorbar(aes(xmin = HL_lo, xmax = HL_hi), width = .25, orientation = "y") +
  geom_point(size = 2.8, colour = col_ref) +
  geom_label(aes(x = HL_hi, label = lab), hjust = -0.05, size = 2.7, label.size = 0, fill = "white", label.padding = unit(0.1, "lines")) +
  scale_x_continuous(expand = expansion(mult = c(.05, .5))) + scale_y_discrete(labels = plab(levels(fdat$Comparison))) +
  labs(x = "Hodges-Lehmann paired difference, error(B) - error(A), 95% CI", y = NULL,
       title = "Paired comparisons (Wilcoxon, Holm p)", subtitle = "right of 0 = A has lower error; brackets = wins/ties/losses of A")

rk <- data.frame(Method = factor(ord, levels = rev(ord)), R = R[ord])
pc <- ggplot(rk, aes(R, Method)) +
  geom_errorbar(aes(xmin = R - CD / 2, xmax = R + CD / 2), width = .25, orientation = "y", colour = "grey35") +
  geom_point(aes(colour = Method == REF), size = 3) +
  geom_text(aes(label = sprintf("%.2f", R)), vjust = -1.1, size = 2.8) +
  scale_colour_manual(values = c("TRUE" = col_ref, "FALSE" = col_oth), guide = "none") + scale_y_discrete(labels = mlab(levels(rk$Method))) +
  labs(x = "Mean rank (1 = lowest error)", y = NULL, title = "Mean ranks and Nemenyi CD",
       subtitle = sprintf("bars = +/- CD/2 (CD = %.2f): non-overlapping bars differ at alpha = %.2f", CD, ALPHA))

txt <- sprintf("Friedman chi2(%d) = %.2f, p = %s   |   Iman-Davenport F = %.2f, p = %s   |   Kendall W = %.2f",
               K - 1, chi, fp(fr$p.value), FF, fp(p_ID), W)

dl <- do.call(rbind, lapply(seq_len(nrow(pairs)), function(i)
  data.frame(DATASET = rownames(X), Comparison = paste(pairs[i, 1], "vs", pairs[i, 2]),
             Diff = X[, pairs[i, 2]] - X[, pairs[i, 1]]))) |>
  mutate(Comparison = factor(Comparison, levels = as.character(pw$Comparison)))
lab_df <- data.frame(Comparison = factor(as.character(pw$Comparison), levels = levels(dl$Comparison)),
                     lab = pw$Signif, y = tapply(dl$Diff, dl$Comparison, max)[as.character(pw$Comparison)])
p2b <- ggplot(dl, aes(Comparison, Diff)) +
  geom_hline(yintercept = 0, linetype = 2) +
  geom_violin(fill = "grey92", colour = "grey60", scale = "width") +
  geom_boxplot(width = .12, outlier.shape = NA, fill = "white") +
  geom_jitter(aes(colour = Diff > 0), width = .08, size = 1.2) +
  geom_text(data = lab_df, aes(y = y, label = lab), vjust = -0.6, fontface = "bold", size = 3.2) +
  scale_colour_manual(values = c("TRUE" = "#2E86C1", "FALSE" = col_ref),
                      labels = c("TRUE" = "A better", "FALSE" = "B better"), name = NULL) +
  scale_y_continuous(expand = expansion(mult = c(.05, .12))) + scale_x_discrete(labels = plab(levels(dl$Comparison))) +
  labs(x = NULL, y = "error(B) - error(A)  [percentage points]",
       title = "Per-dataset paired differences", subtitle = "stars = Holm-adjusted Wilcoxon significance") +
  theme(legend.position = "bottom", axis.text.x = element_text(angle = 20, hjust = 1))

if (!exists("txt2")) txt2 <- NULL
pdd <- p2b
png(paste0(OUT_PREFIX, "_summary.png"), width = 13, height = 9.5, units = "in", res = DPI)
grid.newpage()
gp_t <- gpar(fontface = "bold", fontsize = 11, col = "#1F3864")
if (is.null(txt2)) {
  grid.text(txt, x = 0.012, y = 0.972, just = c("left", "center"), gp = gp_t)
} else {
  grid.text(txt,  x = 0.012, y = 0.985, just = c("left", "center"), gp = gp_t)
  grid.text(txt2, x = 0.012, y = 0.955, just = c("left", "center"), gp = gp_t)
}
place(pa,  0,   .465, .47, .46); place(pb,  .47, .465, .53, .46)
place(pc,  0,   0,    .47, .465); place(pdd, .47, 0,    .53, .465)
dev.off()

cat("\n", txt, "\n"); print(data.frame(pw[, c("Comparison", "Wins_A", "Ties", "Wins_B", "MeanDiff", "p_Wilcoxon", "p_Holm", "r_effect", "Signif")]), row.names = FALSE)
cat("\nDone. Files written with prefix:", OUT_PREFIX, "\n")
