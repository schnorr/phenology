library(tidyverse)
library(colorspace)
library(phenovisr)

# Puts the description of all the masks in a tibble. Originally this description included masks
# from 'data/d0', but to keep things fast I'll just use the community mask.
df.masks <- tibble(Mask.Filename=list.files("data/70", recursive=TRUE, pattern="jpg", full.names=TRUE)) %>%
  separate(Mask.Filename, sep="/", into=c("A", "B", "C", "Mask"), remove=FALSE) %>%
  mutate(Mask = gsub(".jpg", "", Mask)) %>%
  mutate(Mask = gsub("-", "_", Mask)) %>%
  select(-A, -B, -C)

# Puts the description of all the images in another tibble
df.peg <- tibble(Picture.Filename=list.files("Output_PEG", pattern="jpg", recursive=TRUE, full.names=TRUE)) %>%
  separate(Picture.Filename, sep="/", into=c("A", "B", "Picture"), remove=FALSE) %>% select(-A, -B) %>%
  mutate(Picture = gsub(".jpg", "", Picture))

# Now let's extract some data from the image name
df.peg <- df.peg %>% 
  mutate(Separator = Picture) %>%
  separate(Separator, sep='_', into=c('Year', 'Day', 'Hour', 'Sequence')) %>%
  mutate(Sequence = as.numeric(Sequence)) %>%
  mutate(Hour = as.numeric(Hour))

# Since we will only plot the first sequence of each day for a select time range,
# get rid of the other images (for now)
df.peg.selected <- df.peg %>%
  filter(Sequence == 1) %>%
  filter(Hour == 10) %>%
  select(Picture.Filename, Picture)

# Joining pictures to masks...
df.peg.selected.masks <- df.peg.selected %>%
  mutate(dummy = TRUE) %>%
  left_join(df.masks %>% mutate(dummy = TRUE), by = c('dummy')) %>%
  select(-dummy)

# Calculate the histogram for all the images (warning: this can take a potentially long time)
gethist <- function(df, grain=10) {
  mask <- df %>% slice(1) %>% pull(Mask.Filename)
  phenovis_read_mask(mask)
  phenovis_get_HSV_double_histogram(phenovis_H(), df %>% pull(Picture.Filename), grain) %>%
    filter(H != 0) %>%
    as_tibble()
}
df.histograms <- df.peg.selected.masks %>%
  group_by(Mask) %>%
  do(gethist(.)) %>%
  ungroup()

# Tidy the data
df.histograms.tidy <- df.histograms %>%
  rename(H.count = Count) %>%
  # V histogram is spread, let's gather it
  gather(V, V.count, -Mask, -Name, -Width, -Height, -Pixels, -H, -H.count) %>%
  # Remove the V, convert to integer
  mutate(V = as.integer(gsub('V', '', V))) %>%
  # Map to [0,1], convert to double
  mutate(V = as.numeric(round(V/9, 2))) %>%
  # Cosmetics
  separate(Name, sep = '/', into = c('Dir', 'Year', 'Filename')) %>%
  select(-Dir, -Year) %>%
  separate(Filename, sep = '_', into = c('Year', 'Day', 'Hour', 'Seq'), convert = TRUE) %>%
  mutate(Seq = as.integer(gsub('.jpg', '', Seq)))

# Create HSV color palette
palette <- expand.grid(H = seq(0, 359), S = 1, V = seq(0, 9)/10) %>%
  mutate(Color.Code = as.integer((H+V)*10)) %>%
  mutate(Color = hex(HSV(H, S, V))) %>%
  as.tibble
palette.list <- as.character(palette$Color)
names(palette.list) <- palette$Color.Code

df.plot <- df.histograms.tidy %>%
  mutate(Color.Code = as.integer((H+V)*10)) %>%
  group_by(Mask, Year, Day, Hour, Seq, Width, Height, Pixels, H) %>%
  # Compute the maximum V.count (that is, the mode)
  mutate(V.max.count = max(V.count)) %>%
  # Get only that maximum among other Vs
  filter(V.count == V.max.count) %>%
  # Get rid of the computed value
  select(-V.max.count) %>%
  ungroup()

# Plot
df.plot %>%
  ggplot(aes(x = Day, y = H.count, fill=as.factor(Color.Code))) +
  scale_fill_manual(values = palette.list) +
  geom_bar(stat='identity') +
  theme_bw(base_size=16) +
  theme(axis.ticks = element_blank(),
        axis.text = element_blank(),
        plot.margin = unit(c(0,0,0,0), "cm"),
        legend.spacing = unit(0, "mm"),
        panel.grid = element_blank(),
        legend.position = "none",
        legend.justification = "left",
        legend.box.spacing = unit(0, "pt"),
        legend.box.margin = margin(0,0,0,0),
        legend.title = element_blank()) +
  guides(fill = guide_legend(nrow = 1)) +
  xlab("Day of the Year (2014)") +
  ylab("Normalized size of bins")

