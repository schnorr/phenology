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

# Since we will only plot the first sequence of each day for a select time range,
# get rid of the other images (for now)
df.peg <- df.peg %>% 
  mutate(Separator = Picture) %>%
  separate(Separator, sep='_', into=c('Year', 'Day', 'Hour', 'Sequence')) %>%
  mutate(Sequence = as.numeric(Sequence)) %>%
  mutate(Hour = as.numeric(Hour)) %>%
  filter(Sequence == 1) %>%
  filter(Hour >= 8, Hour <= 17) %>%
  # To make it even faster, get only the images from 10am
  filter(Hour == 10) %>%
  select(Picture.Filename, Picture)

# Calculate the histogram for all the images (warning: this takes 10m+)
gethist <- function(df, grain=10) {
  mask <- df %>% slice(1) %>% pull(Mask.Filename);
  phenovis_read_mask(mask);
  phenovis_get_HSV_double_histogram(phenovis_H(), df %>% pull(Picture.Filename), grain) %>%
    as_tibble()
}

df.histograms <- df.masks %>%
  mutate(dummy=TRUE) %>%
  group_by(Mask) %>%
  left_join(df.peg %>% mutate(dummy=TRUE), by=c("dummy")) %>%   
  select(-dummy) %>%
  do(gethist(.)) %>%
  ungroup()

# Prepare data for stacked bar view.
df <- df.histograms %>%
  gather(variable, value, -Mask, -Name, -Width, -Height, -Pixels, -H, -Count) %>%
  mutate(variable = as.integer(substr(as.character(variable), 2, 100))) %>%
  mutate(variable = variable*360 + H) %>%
  separate(Name, sep="/", into=c("Dir", "Year", "Filename")) %>%
  select(-Dir, -Year) %>%
  separate(Filename, sep="_", into=c("Year", "Day", "Hour", "Sequence"), convert=TRUE) %>%
  mutate(Sequence = gsub(".jpg", "", Sequence))


# Create HSV color palette
palette <- expand.grid(V = seq(0, 9), H = seq(0, 359), S = 1) %>% mutate(Color = hex(HSV(H, S, V/10)))

# Plot it...
lowLimit = 0;
highLimit = 3600;

df %>% 
  # This is not necessary for HSV
  #filter(variable >= lowLimit, variable < highLimit) %>%
  filter(value != 0) %>%
  filter(Hour >= 8, Hour <= 17) %>%
  filter(Sequence == 1) %>%
  group_by(Mask) %>%
  mutate(value = value/Count) %>%
  ungroup() %>%
  ggplot(aes(x = Day, y = value, fill=as.factor(variable))) +
  geom_bar(stat='identity', width=1) +
  ylim(0,NA) +
  theme_bw (base_size=16) +
  xlab("Day of the Year (2014)") +
  ylab("Normalized size of bins") +
  scale_fill_manual(values=palette$Color) +
  theme(#axis.ticks = element_blank(),
    #axis.text = element_blank(),
    plot.margin = unit(c(0,0,0,0), "cm"),
    legend.spacing = unit(1, "mm"),
    panel.grid = element_blank(),
    legend.position = "top",
    legend.justification = "left",
    legend.box.spacing = unit(0, "pt"),
    legend.box.margin = margin(0,0,0,0),
    legend.title = element_blank()) +
  guides(fill = guide_legend(nrow = 1)) +
  facet_grid(Hour~Mask, scales="free")
